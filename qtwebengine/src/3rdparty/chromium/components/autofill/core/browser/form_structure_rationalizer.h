// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_STRUCTURE_RATIONALIZER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_STRUCTURE_RATIONALIZER_H_

#include <memory>
#include <set>
#include <vector>

#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/metrics/autofill_metrics.h"
#include "components/autofill/core/common/form_field_data.h"

namespace autofill {

class LogManager;

// Rationalization is the process of taking a parsed form structure and
// changing the types of fields based on their context based on assumptions
// about how forms should be structured.
class FormStructureRationalizer {
 public:
  // `fields` must outlive the FormStructureRationalizer and must not be null.
  FormStructureRationalizer(std::vector<std::unique_ptr<AutofillField>>* fields,
                            FormSignature form_signature);
  ~FormStructureRationalizer();
  FormStructureRationalizer(const FormStructureRationalizer&) = delete;
  FormStructureRationalizer& operator=(const FormStructureRationalizer&) =
      delete;

  // Tunes the fields with identical predictions.
  void RationalizeRepeatedFields(AutofillMetrics::FormInteractionsUkmLogger*,
                                 LogManager* log_manager);

  // A helper function to review the predictions and do appropriate adjustments
  // when it considers necessary.
  void RationalizeFieldTypePredictions(LogManager* log_manager);

  // Rationalize phone number fields in a given section, that is only fill
  // the fields that are considered composing a first complete phone number.
  void RationalizePhoneNumbersInSection(const Section& section);

 private:
  friend class FormStructureTestApi;

  // This class wraps a vector of vectors of field indices. The indices of a
  // vector belong to the same group.
  class SectionedFieldsIndexes;

  // A function to fine tune the credit cards related predictions. For example:
  // lone credit card fields in an otherwise non-credit-card related form is
  // unlikely to be correct, the function will override that prediction.
  void RationalizeCreditCardFieldPredictions(LogManager* log_manager);

  // A function to rewrite sequences of (street address, address_line2) into
  // (address_line1, address_line2) as server predictions sometimes introduce
  // wrong street address predictions.
  void RationalizeStreetAddressAndAddressLine(LogManager* log_manager);

  // The rationalization is based on the visible fields, but should be applied
  // to the hidden select fields. This is because hidden 'select' fields are
  // also autofilled to take care of the synthetic fields.
  void ApplyRationalizationsToHiddenSelects(
      size_t field_index,
      ServerFieldType new_type,
      AutofillMetrics::FormInteractionsUkmLogger*);

  // Returns true if we can replace server predictions with the heuristics one.
  bool HeuristicsPredictionsAreApplicable(size_t upper_index,
                                          size_t lower_index,
                                          ServerFieldType first_type,
                                          ServerFieldType second_type);

  // Applies upper type to upper field, and lower type to lower field, and
  // applies the rationalization also to hidden select fields if necessary.
  void ApplyRationalizationsToFields(
      size_t upper_index,
      size_t lower_index,
      ServerFieldType upper_type,
      ServerFieldType lower_type,
      AutofillMetrics::FormInteractionsUkmLogger*);

  // Returns true if the fields_[index] server type should be rationalized to
  // ADDRESS_HOME_COUNTRY.
  bool FieldShouldBeRationalizedToCountry(size_t index);

  // Set fields_[|field_index|] to |new_type| and log this change.
  void ApplyRationalizationsToFieldAndLog(
      size_t field_index,
      ServerFieldType new_type,
      AutofillMetrics::FormInteractionsUkmLogger* form_interactions_ukm_logger);

  // Two or three fields predicted as the whole address should be address lines
  // 1, 2 and 3 instead.
  void RationalizeAddressLineFields(
      SectionedFieldsIndexes* sections_of_address_indexes,
      AutofillMetrics::FormInteractionsUkmLogger*,
      LogManager* log_manager);

  // Rationalize state and country interdependently.
  void RationalizeAddressStateCountry(
      SectionedFieldsIndexes* sections_of_state_indexes,
      SectionedFieldsIndexes* sections_of_country_indexes,
      AutofillMetrics::FormInteractionsUkmLogger*,
      LogManager* log_manager);

  // Filters out fields that don't meet the relationship ruleset for their type
  // defined in |type_relationships_rules_|.
  void RationalizeTypeRelationships(LogManager* log_manager);

  // A vector of all the input fields in the form. The reference is const but
  // the fields are mutable by design.
  const std::vector<std::unique_ptr<AutofillField>>& fields_;

  // Signature for the rationalized form. Required for logging.
  const FormSignature form_signature_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_STRUCTURE_RATIONALIZER_H_
