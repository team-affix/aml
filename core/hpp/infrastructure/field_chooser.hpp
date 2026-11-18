#ifndef FIELD_CHOOSER_HPP
#define FIELD_CHOOSER_HPP

#include <utility>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/var_id.hpp"
#include "debug_assert.hpp"

// Chooses the field a funnelling match descends into: one of a bin's heterogeneous variables from `first_field` on,
// which are the fields the match above just bound.
template<typename IFindVariableCandidates, typename IChooseField>
struct field_chooser {
    field_chooser(IFindVariableCandidates& find_variable_candidates, IChooseField& choose_field);
    candidate choose(const bin& data, var_id first_field);
private:
    IFindVariableCandidates& find_variable_candidates_;
    IChooseField& choose_field_;
};

#define FIELD_CHOOSER_TEMPLATE template<typename IFindVariableCandidates, typename IChooseField>
#define FIELD_CHOOSER          field_chooser<IFindVariableCandidates, IChooseField>

FIELD_CHOOSER_TEMPLATE
FIELD_CHOOSER::field_chooser(IFindVariableCandidates& find_variable_candidates, IChooseField& choose_field)
    : find_variable_candidates_(find_variable_candidates)
    , choose_field_(choose_field) {}

FIELD_CHOOSER_TEMPLATE
candidate FIELD_CHOOSER::choose(const bin& data, var_id first_field) {
    std::vector<candidate> fields = find_variable_candidates_.find(data, first_field);
    DEBUG_ASSERT(!fields.empty());
    return std::move(fields.at(choose_field_.choose(fields.size())));
}

#undef FIELD_CHOOSER
#undef FIELD_CHOOSER_TEMPLATE

#endif
