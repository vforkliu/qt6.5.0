// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/mdns/mdns_api.h"

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/media/router/test/mock_dns_sd_registry.h"
#include "chrome/common/extensions/api/mdns.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/mock_render_process_host.h"
#include "extensions/browser/event_listener_map.h"
#include "extensions/browser/event_router_factory.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/manifest_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using media_router::MockDnsSdRegistry;
using testing::_;
using testing::Return;
using testing::ReturnRef;

namespace extensions {
namespace {

const char kExtId[] = "mbflcebpggnecokmikipoihdbecnjfoj";
const char kService1[] = "service1";
const char kService2[] = "service2";

// Registers a new EventListener for |service_types| in |listener_list|.
void AddEventListener(
    const std::string& extension_id,
    const std::string& service_type,
    extensions::EventListenerMap::ListenerList* listener_list) {
  std::unique_ptr<base::DictionaryValue> filter(new base::DictionaryValue);
  filter->SetStringKey(kEventFilterServiceTypeKey, service_type);
  listener_list->push_back(EventListener::ForExtension(
      kEventFilterServiceTypeKey, extension_id, nullptr, std::move(filter)));
}

class NullDelegate : public EventListenerMap::Delegate {
 public:
  void OnListenerAdded(const EventListener* listener) override {}
  void OnListenerRemoved(const EventListener* listener) override {}
};

// Testing subclass of MDnsAPI which replaces calls to core browser components
// that we don't want to have to instantiate or mock for these tests.
class MockedMDnsAPI : public MDnsAPI {
 public:
  explicit MockedMDnsAPI(content::BrowserContext* context) : MDnsAPI(context) {}

 public:
  MOCK_CONST_METHOD2(IsMDnsAllowed,
                     bool(const std::string& extension_id,
                          const std::string& service_type));

  MOCK_METHOD0(GetEventListeners,
               const extensions::EventListenerMap::ListenerList&());
};

std::unique_ptr<KeyedService> MockedMDnsAPITestingFactoryFunction(
    content::BrowserContext* context) {
  return std::make_unique<MockedMDnsAPI>(context);
}

std::unique_ptr<KeyedService> MDnsAPITestingFactoryFunction(
    content::BrowserContext* context) {
  return std::make_unique<MDnsAPI>(context);
}

std::unique_ptr<KeyedService> BuildEventRouter(
    content::BrowserContext* context) {
  return std::make_unique<extensions::EventRouter>(
      context, ExtensionPrefs::Get(context));
}

// For ExtensionService interface when it requires a path that is not used.
base::FilePath bogus_file_pathname(const std::string& name) {
  return base::FilePath(FILE_PATH_LITERAL("//foobar_nonexistent"))
      .AppendASCII(name);
}

class MockEventRouter : public EventRouter {
 public:
  explicit MockEventRouter(content::BrowserContext* browser_context,
                           ExtensionPrefs* extension_prefs)
      : EventRouter(browser_context, extension_prefs) {}
  ~MockEventRouter() override {}

  void BroadcastEvent(std::unique_ptr<Event> event) override {
    BroadcastEventPtr(event.get());
  }
  MOCK_METHOD1(BroadcastEventPtr, void(Event* event));
};

std::unique_ptr<KeyedService> MockEventRouterFactoryFunction(
    content::BrowserContext* context) {
  return std::make_unique<MockEventRouter>(context,
                                           ExtensionPrefs::Get(context));
}

class EventServiceListSizeMatcher
    : public testing::MatcherInterface<const Event&> {
 public:
  explicit EventServiceListSizeMatcher(size_t expected_size)
      : expected_size_(expected_size) {}

  virtual bool MatchAndExplain(const Event& e,
                               testing::MatchResultListener* listener) const {
    if (e.event_args.size() != 1) {
      *listener << "event.event_arg.GetSize() should be 1 but is "
                << e.event_args.size();
      return false;
    }
    const base::ListValue* services = nullptr;
    {
      const base::Value& out = e.event_args[0];
      services = static_cast<const base::ListValue*>(&out);
    }
    if (!services) {
      *listener << "event's service list argument is not a ListValue";
      return false;
    }
    *listener << "number of services is " << services->GetList().size();
    return static_cast<testing::Matcher<size_t>>(testing::Eq(expected_size_))
        .MatchAndExplain(services->GetList().size(), listener);
  }

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "is an onServiceList event where the number of services is "
        << expected_size_;
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "isn't an onServiceList event where the number of services is "
        << expected_size_;
  }

 private:
  size_t expected_size_;
};

inline testing::Matcher<const Event&> EventServiceListSize(
    size_t expected_size) {
  return testing::MakeMatcher(new EventServiceListSizeMatcher(expected_size));
}

}  // namespace

class MDnsAPITest : public extensions::ExtensionServiceTestBase {
 public:
  void SetUp() override {
    extensions::ExtensionServiceTestBase::SetUp();

    // Set up browser_context().
    InitializeEmptyExtensionService();

    // A custom TestingFactoryFunction is required for an MDnsAPI to actually be
    // constructed.
    MDnsAPI::GetFactoryInstance()->SetTestingFactory(browser_context(),
                                                     GetMDnsFactory());

    EventRouterFactory::GetInstance()->SetTestingFactory(
        browser_context(), base::BindRepeating(&BuildEventRouter));

    // Do some sanity checking
    ASSERT_TRUE(MDnsAPI::Get(browser_context()));      // constructs MDnsAPI
    ASSERT_TRUE(EventRouter::Get(browser_context()));  // constructs EventRouter

    registry_ = std::make_unique<testing::NiceMock<MockDnsSdRegistry>>(
        MDnsAPI::Get(browser_context()));
    EXPECT_CALL(*dns_sd_registry(),
                AddObserver(MDnsAPI::Get(browser_context())))
        .Times(1);
    MDnsAPI::Get(browser_context())
        ->SetDnsSdRegistryForTesting(registry_.get());

    render_process_host_ =
        std::make_unique<content::MockRenderProcessHost>(browser_context());
  }

  // Returns the mDNS API factory function (mock vs. real) to use for the test.
  virtual BrowserContextKeyedServiceFactory::TestingFactory GetMDnsFactory() {
    return base::BindRepeating(&MDnsAPITestingFactoryFunction);
  }

  void TearDown() override {
    MDnsAPI::Get(browser_context())->SetDnsSdRegistryForTesting(nullptr);
    render_process_host_.reset();
    extensions::ExtensionServiceTestBase::TearDown();
  }

  virtual MockDnsSdRegistry* dns_sd_registry() { return registry_.get(); }

  // Constructs an extension according to the parameters that matter most to
  // MDnsAPI the local unit tests.
  const scoped_refptr<extensions::Extension> CreateExtension(
      std::string name,
      bool is_platform_app,
      std::string extension_id) {
    base::DictionaryValue manifest;
    manifest.SetStringKey(extensions::manifest_keys::kVersion, "1.0.0.0");
    manifest.SetStringKey(extensions::manifest_keys::kName, name);
    manifest.SetIntKey(extensions::manifest_keys::kManifestVersion, 2);
    if (is_platform_app) {
      // Setting app.background.page = "background.html" is sufficient to make
      // the extension type TYPE_PLATFORM_APP.
      manifest.Set(extensions::manifest_keys::kPlatformAppBackgroundPage,
                   std::make_unique<base::Value>("background.html"));
    }

    std::string error;
    return extensions::Extension::Create(
        bogus_file_pathname(name),
        extensions::mojom::ManifestLocation::kInvalidLocation, manifest,
        Extension::NO_FLAGS, extension_id, &error);
  }

  content::RenderProcessHost* render_process_host() const {
    return render_process_host_.get();
  }

 private:
  std::unique_ptr<testing::NiceMock<MockDnsSdRegistry>> registry_;

  std::unique_ptr<content::RenderProcessHost> render_process_host_;
};

class MDnsAPIMaxServicesTest : public MDnsAPITest {
 public:
  MockEventRouter* event_router() {
    return static_cast<MockEventRouter*>(EventRouter::Get(browser_context()));
  }
};

class MDnsAPIDiscoveryTest : public MDnsAPITest {
 public:
  BrowserContextKeyedServiceFactory::TestingFactory GetMDnsFactory() override {
    return base::BindRepeating(&MockedMDnsAPITestingFactoryFunction);
  }

  void SetUp() override {
    MDnsAPITest::SetUp();
    mdns_api_ = static_cast<MockedMDnsAPI*>(MDnsAPI::Get(browser_context()));
    EXPECT_CALL(*mdns_api_, IsMDnsAllowed(_, _)).WillRepeatedly(Return(true));
  }

 protected:
  raw_ptr<MockedMDnsAPI> mdns_api_;
};

class MDnsAPIExtensionTest
    : public MDnsAPITest,
      public testing::WithParamInterface<version_info::Channel> {};

TEST_F(MDnsAPIDiscoveryTest, ServiceListenersAddedAndRemoved) {
  EventRouterFactory::GetInstance()->SetTestingFactory(
      browser_context(), base::BindRepeating(&MockEventRouterFactoryFunction));
  extensions::EventListenerMap::ListenerList listeners;

  extensions::EventListenerInfo listener_info(
      kEventFilterServiceTypeKey, kExtId, GURL(), browser_context());

  EXPECT_CALL(*mdns_api_, GetEventListeners())
      .WillRepeatedly(ReturnRef(listeners));

  // Listener #1 added with kService1.
  AddEventListener(kExtId, kService1, &listeners);
  EXPECT_CALL(*dns_sd_registry(), RegisterDnsSdListener(kService1));
  mdns_api_->OnListenerAdded(listener_info);

  // Listener #2 added with kService2.
  AddEventListener(kExtId, kService2, &listeners);
  EXPECT_CALL(*dns_sd_registry(), RegisterDnsSdListener(kService2));
  mdns_api_->OnListenerAdded(listener_info);

  // No-op.
  mdns_api_->OnListenerAdded(listener_info);

  // Listener #3 added with kService2. Should trigger a refresh of kService2.
  AddEventListener(kExtId, kService2, &listeners);
  EXPECT_CALL(*dns_sd_registry(), Publish(kService2));
  mdns_api_->OnListenerAdded(listener_info);

  // Listener #3 removed, should be a no-op since there is still a live
  // listener for kService2.
  listeners.pop_back();
  mdns_api_->OnListenerRemoved(listener_info);

  // Listener #2 removed, should unregister kService2.
  listeners.pop_back();
  EXPECT_CALL(*dns_sd_registry(), UnregisterDnsSdListener(kService2));
  mdns_api_->OnListenerRemoved(listener_info);

  // Listener #1 removed, should unregister kService1.
  listeners.pop_back();
  EXPECT_CALL(*dns_sd_registry(), UnregisterDnsSdListener(kService1));
  mdns_api_->OnListenerRemoved(listener_info);

  // No-op.
  mdns_api_->OnListenerAdded(listener_info);

  // Listener #4 added with kService1.
  AddEventListener(kExtId, kService1, &listeners);
  EXPECT_CALL(*dns_sd_registry(), RegisterDnsSdListener(kService1));
  mdns_api_->OnListenerAdded(listener_info);
}

TEST_F(MDnsAPIMaxServicesTest, OnServiceListDoesNotExceedLimit) {
  EventRouterFactory::GetInstance()->SetTestingFactory(
      browser_context(), base::BindRepeating(&MockEventRouterFactoryFunction));

  // This check should change when the [value=2048] changes in the IDL file.
  EXPECT_EQ(2048, api::mdns::MAX_SERVICE_INSTANCES_PER_EVENT);

  // Dispatch an mDNS event with more service instances than the max, and ensure
  // that the list is truncated by inspecting the argument to MockEventRouter's
  // BroadcastEvent method.
  media_router::DnsSdRegistry::DnsSdServiceList services;
  for (int i = 0; i < api::mdns::MAX_SERVICE_INSTANCES_PER_EVENT + 10; ++i) {
    services.push_back(media_router::DnsSdService());
  }
  EXPECT_CALL(
      *event_router(),
      BroadcastEventPtr(testing::Pointee(EventServiceListSize(
          static_cast<size_t>(api::mdns::MAX_SERVICE_INSTANCES_PER_EVENT)))))
      .Times(1);
  dns_sd_registry()->DispatchMDnsEvent("_testing._tcp.local", services);
}

TEST_P(MDnsAPIExtensionTest, ExtensionRespectsAllowlist) {
  const bool is_dev = GetParam() == version_info::Channel::DEV;
  extensions::ScopedCurrentChannel channel_override(GetParam());

  scoped_refptr<extensions::Extension> extension =
      CreateExtension("Dinosaur networker", false, kExtId);
  ExtensionRegistry::Get(browser_context())->AddEnabled(extension);
  ASSERT_EQ(Manifest::TYPE_EXTENSION, extension->GetType());
  auto param = mojom::EventListenerParam::NewExtensionId(kExtId);

  // There is a allowlist of mdns service types extensions may access, which
  // includes "_testing._tcp.local" and excludes "_trex._tcp.local"
  {
    base::DictionaryValue filter;
    filter.SetStringKey(kEventFilterServiceTypeKey, "_trex._tcp.local");

    ASSERT_TRUE(dns_sd_registry());
    // Test that the extension is not able to listen to a non-allowlisted
    // service, unless we are on dev channel.
    EXPECT_CALL(*dns_sd_registry(), RegisterDnsSdListener("_trex._tcp.local"))
        .Times(is_dev ? 1 : 0);
    EventRouter::Get(browser_context())
        ->AddFilteredEventListener(api::mdns::OnServiceList::kEventName,
                                   render_process_host(), param.Clone(),
                                   absl::nullopt, filter, false);

    EXPECT_CALL(*dns_sd_registry(), UnregisterDnsSdListener("_trex._tcp.local"))
        .Times(is_dev ? 1 : 0);
    EventRouter::Get(browser_context())
        ->RemoveFilteredEventListener(api::mdns::OnServiceList::kEventName,
                                      render_process_host(), param.Clone(),
                                      absl::nullopt, filter, false);
  }
  {
    base::DictionaryValue filter;
    filter.SetStringKey(kEventFilterServiceTypeKey, "_testing._tcp.local");

    ASSERT_TRUE(dns_sd_registry());
    // Test that the extension is able to listen to a allowlisted service
    EXPECT_CALL(*dns_sd_registry(),
                RegisterDnsSdListener("_testing._tcp.local"));
    EventRouter::Get(browser_context())
        ->AddFilteredEventListener(api::mdns::OnServiceList::kEventName,
                                   render_process_host(), param.Clone(),
                                   absl::nullopt, filter, false);

    EXPECT_CALL(*dns_sd_registry(),
                UnregisterDnsSdListener("_testing._tcp.local"));
    EventRouter::Get(browser_context())
        ->RemoveFilteredEventListener(api::mdns::OnServiceList::kEventName,
                                      render_process_host(), param.Clone(),
                                      absl::nullopt, filter, false);
  }
}

INSTANTIATE_TEST_SUITE_P(Channels,
                         MDnsAPIExtensionTest,
                         testing::Values(version_info::Channel::DEV,
                                         version_info::Channel::STABLE));

TEST_F(MDnsAPITest, PlatformAppsNotSubjectToAllowlist) {
  scoped_refptr<extensions::Extension> extension =
      CreateExtension("Dinosaur networker", true, kExtId);
  ExtensionRegistry::Get(browser_context())->AddEnabled(extension);
  ASSERT_TRUE(extension->is_platform_app());
  auto param = mojom::EventListenerParam::NewExtensionId(kExtId);

  base::DictionaryValue filter;
  filter.SetStringKey(kEventFilterServiceTypeKey, "_trex._tcp.local");

  ASSERT_TRUE(dns_sd_registry());
  // Test that the extension is able to listen to a non-allowlisted service
  EXPECT_CALL(*dns_sd_registry(), RegisterDnsSdListener("_trex._tcp.local"));

  EventRouter::Get(browser_context())
      ->AddFilteredEventListener(api::mdns::OnServiceList::kEventName,
                                 render_process_host(), param.Clone(),
                                 absl::nullopt, filter, false);

  EXPECT_CALL(*dns_sd_registry(), UnregisterDnsSdListener("_trex._tcp.local"));
  EventRouter::Get(browser_context())
      ->RemoveFilteredEventListener(api::mdns::OnServiceList::kEventName,
                                    render_process_host(), param.Clone(),
                                    absl::nullopt, filter, false);
}

}  // namespace extensions
