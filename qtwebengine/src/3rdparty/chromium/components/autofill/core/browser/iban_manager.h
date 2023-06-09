// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_IBAN_MANAGER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_IBAN_MANAGER_H_

#include "base/gtest_prod_util.h"
#include "components/autofill/core/browser/autofill_subject.h"
#include "components/autofill/core/browser/data_model/iban.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/single_field_form_filler.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/webdata/common/web_data_service_consumer.h"

namespace autofill {

class PersonalDataManager;
struct SuggestionsContext;

// Per-profile IBAN Manager. This class handles IBAN-related functionality
// such as retrieving IBAN data, managing IBAN suggestions, filling IBAN fields,
// and handling form submission data when there is an IBAN field present.
class IBANManager : public SingleFieldFormFiller,
                    public KeyedService,
                    public AutofillSubject {
 public:
  // Initializes the instance with the given parameters. |personal_data_manager|
  // is a profile-scope data manager used to retrieve IBAN data from the
  // local autofill table. |is_off_the_record| indicates whether the user is
  // currently operating in an off-the-record context (i.e. incognito).
  explicit IBANManager(PersonalDataManager* personal_data_manager,
                       bool is_off_the_record);

  IBANManager(const IBANManager&) = delete;
  IBANManager& operator=(const IBANManager&) = delete;

  ~IBANManager() override;

  // SingleFieldFormFiller overrides:
  [[nodiscard]] bool OnGetSingleFieldSuggestions(
      int query_id,
      bool is_autocomplete_enabled,
      bool autoselect_first_suggestion,
      const FormFieldData& field,
      base::WeakPtr<SuggestionsHandler> handler,
      const SuggestionsContext& context) override;
  void OnWillSubmitFormWithFields(const std::vector<FormFieldData>& fields,
                                  bool is_autocomplete_enabled) override {}
  void CancelPendingQueries(const SuggestionsHandler* handler) override {}
  void OnRemoveCurrentSingleFieldSuggestion(const std::u16string& field_name,
                                            const std::u16string& value,
                                            int frontend_id) override {}
  void OnSingleFieldSuggestionSelected(const std::u16string& value,
                                       int frontend_id) override {}

  base::WeakPtr<IBANManager> GetWeakPtr();

#if defined(UNIT_TEST)
  // Assign types to the fields for the testing purposes.
  void SetOffTheRecordForTesting(bool is_off_the_record) {
    is_off_the_record_ = is_off_the_record;
  }
#endif

 private:
  // Sends suggestions for |ibans| to the |query_handler|'s handler for display
  // in the associated Autofill popup.
  void SendIBANSuggestions(const std::vector<IBAN*>& ibans,
                           const QueryHandler& query_handler);

  PersonalDataManager* personal_data_manager_ = nullptr;

  bool is_off_the_record_ = false;

  base::WeakPtrFactory<IBANManager> weak_ptr_factory_{this};
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_IBAN_MANAGER_H_
