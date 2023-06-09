// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_LOGGING_LOG_ROUTER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_LOGGING_LOG_ROUTER_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"

namespace autofill {

class RoutingLogManager;
class LogReceiver;

// The router stands between LogManager and LogReceiver instances. Both managers
// and receivers need to register (and unregister) with the router. After that,
// the following communication is enabled:
//   * LogManagers are notified when logging starts or stops being possible
//   * LogReceivers are sent logs routed through LogRouter
class LogRouter : public KeyedService {
 public:
  LogRouter();

  LogRouter(const LogRouter&) = delete;
  LogRouter& operator=(const LogRouter&) = delete;

  ~LogRouter() override;

  // Returns a JSON entry that can be fed into the logger.
  static base::Value::Dict CreateEntryForText(const std::string& text);

  // Passes logs to the router. Only call when there are receivers registered.
  void ProcessLog(const std::string& text);
  void ProcessLog(const base::Value::Dict& node);

  // All four (Unr|R)egister* methods below are safe to call from the
  // constructor of the registered object, because they do not call that object,
  // and the router only runs on a single thread.

  // The managers must register to be notified about whether there are some
  // receivers or not. RegisterManager adds |manager| to the right observer list
  // and returns true iff there are some receivers registered.
  bool RegisterManager(RoutingLogManager* manager);
  // Remove |manager| from the observers list.
  void UnregisterManager(RoutingLogManager* manager);

  // The receivers must register to get updates with new logs in the future.
  void RegisterReceiver(LogReceiver* receiver);
  // Remove |receiver| from the observers list.
  void UnregisterReceiver(LogReceiver* receiver);

 private:
  // Observer lists for managers and receivers. The |true| in the template
  // specialisation means that they will check that all observers were removed
  // on destruction.
  base::ObserverList<RoutingLogManager, true>::Unchecked managers_;
  base::ObserverList<LogReceiver, true>::Unchecked receivers_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_LOGGING_LOG_ROUTER_H_
