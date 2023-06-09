// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/reduce_accept_language/browser/reduce_accept_language_service.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_constraints.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "net/http/http_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace reduce_accept_language {

namespace {

const char kReduceAcceptLanguageSettingKey[] = "reduce-accept-language";

}  // namespace

ReduceAcceptLanguageService::ReduceAcceptLanguageService(
    HostContentSettingsMap* settings_map,
    PrefService* pref_service,
    bool is_incognito)
    : settings_map_(settings_map), is_incognito_(is_incognito) {
  DCHECK(settings_map_);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  const std::string& key = language::prefs::kPreferredLanguages;
#else
  const std::string& key = language::prefs::kAcceptLanguages;
#endif
  pref_accept_language_.Init(
      key, pref_service,
      base::BindRepeating(&ReduceAcceptLanguageService::UpdateAcceptLanguage,
                          base::Unretained(this)));
  // StringPrefMember Init doesn't trigger the callback observer, we need to
  // explicitly set the accept Language after init.
  UpdateAcceptLanguage();
}

ReduceAcceptLanguageService::~ReduceAcceptLanguageService() = default;

absl::optional<std::string> ReduceAcceptLanguageService::GetReducedLanguage(
    const url::Origin& origin) {
  const GURL& url = origin.GetURL();

  // Only reduce accept-language in http and https scheme.
  if (!url.SchemeIsHTTPOrHTTPS())
    return absl::nullopt;

  ContentSettingsForOneType accept_language_rules;
  settings_map_->GetSettingsForOneType(
      ContentSettingsType::REDUCED_ACCEPT_LANGUAGE, &accept_language_rules);

  if (accept_language_rules.empty())
    return absl::nullopt;

  for (const auto& rule : accept_language_rules) {
    // Look for an exact match since persisted reduce accept language are
    // disabled by default, and enabled only on per-origin basis.
    if (rule.primary_pattern == ContentSettingsPattern::Wildcard() ||
        !rule.primary_pattern.Matches(url)) {
      continue;
    }

    // Found an exact match.
    DCHECK(ContentSettingsPattern::Wildcard() == rule.secondary_pattern);
    DCHECK(rule.setting_value.is_dict());

    const base::Value* language_value =
        rule.setting_value.GetDict().Find(kReduceAcceptLanguageSettingKey);
    if (language_value == nullptr)
      continue;

    // We should guarantee reduce accept language always be Type::String since
    // we save it as string in the Prefs.
    DCHECK(language_value->is_string());
    return absl::make_optional(language_value->GetString());
  }
  return absl::nullopt;
}

std::vector<std::string> ReduceAcceptLanguageService::GetUserAcceptLanguages()
    const {
  return user_accept_languages_;
}

void ReduceAcceptLanguageService::PersistReducedLanguage(
    const url::Origin& origin,
    const std::string& language) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const GURL url = origin.GetURL();
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS())
    return;

  const base::TimeTicks start_time = base::TimeTicks::Now();

  base::Value::Dict accept_language_dictionary;
  accept_language_dictionary.Set(kReduceAcceptLanguageSettingKey, language);
  settings_map_->SetWebsiteSettingDefaultScope(
      url, GURL(), ContentSettingsType::REDUCED_ACCEPT_LANGUAGE,
      base::Value(std::move(accept_language_dictionary)),
      {base::Time(), content_settings::SessionModel::Durable});

  // Record the time spent getting the reduce accept language.
  base::TimeDelta duration = base::TimeTicks::Now() - start_time;
  base::UmaHistogramTimes("ReduceAcceptLanguage.StoreLatency", duration);
  base::UmaHistogramCounts100("ReduceAcceptLanguage.UpdateSize",
                              language.size());
}

void ReduceAcceptLanguageService::UpdateAcceptLanguage() {
  // In incognito mode return only the first language.
  std::string accept_languages_str = net::HttpUtil::ExpandLanguageList(
      is_incognito_
          ? language::GetFirstLanguage(pref_accept_language_.GetValue())
          : pref_accept_language_.GetValue());
  user_accept_languages_ = base::SplitString(
      accept_languages_str, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
}

}  // namespace reduce_accept_language