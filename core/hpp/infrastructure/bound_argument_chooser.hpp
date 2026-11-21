#ifndef BOUND_ARGUMENT_CHOOSER_HPP
#define BOUND_ARGUMENT_CHOOSER_HPP

#include <utility>
#include <vector>
#include "value_objects/bin_view.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/var_id.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses the constructor argument a funnel descends into: one of a bin's heterogeneous variables from
// `first_bound_argument` on, which are the arguments the match above just bound.
template<typename IFindVariableCandidates, typename IChooseBoundArgument>
struct bound_argument_chooser {
    bound_argument_chooser(
        IFindVariableCandidates& find_variable_candidates, IChooseBoundArgument& choose_bound_argument);
    candidate choose(const bin_view& data, var_id first_bound_argument);
private:
    IFindVariableCandidates& find_variable_candidates_;
    IChooseBoundArgument& choose_bound_argument_;
};

#define BOUND_ARGUMENT_CHOOSER_TEMPLATE template<typename IFVC, typename ICBA>
#define BOUND_ARGUMENT_CHOOSER          bound_argument_chooser<IFVC, ICBA>

BOUND_ARGUMENT_CHOOSER_TEMPLATE
BOUND_ARGUMENT_CHOOSER::bound_argument_chooser(IFVC& find_variable_candidates, ICBA& choose_bound_argument)
    : find_variable_candidates_(find_variable_candidates)
    , choose_bound_argument_(choose_bound_argument) {}

BOUND_ARGUMENT_CHOOSER_TEMPLATE
candidate BOUND_ARGUMENT_CHOOSER::choose(const bin_view& data, var_id first_bound_argument) {
    std::vector<candidate> arguments = find_variable_candidates_.find(data, first_bound_argument);
    DEBUG_ASSERT(!arguments.empty());
    return std::move(arguments.at(choose_bound_argument_.choose(arguments.size())));
}

#undef BOUND_ARGUMENT_CHOOSER
#undef BOUND_ARGUMENT_CHOOSER_TEMPLATE

}

#endif
