#ifndef FEATURE_SYNTHESIZER_HPP
#define FEATURE_SYNTHESIZER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/bin_view.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Synthesizes heterogeneous features of a bin in one session of at most `max_applications` primitive applications.
// Each step first chooses between stopping (option 0) and continuing (option 1); only on continuing does it choose
// which of the primitives with an application witness to apply. Keeping the step a two-way choice keeps MCTS's
// branching small there. The session also stops at the cap, once no primitive has a witness, or once a feature's
// column is the label column (it can be the bin's output). The features returned are the session's own: they hold
// until the next session starts.
template<
    typename IStartSession,
    typename ICountApplications,
    typename IFindPrimitiveOptions,
    typename IChooseStep,
    typename IChoosePrimitive,
    typename IApplyChosenPrimitive,
    typename IGetFeatures,
    typename IFindOutput>
struct feature_synthesizer {
    feature_synthesizer(
        IStartSession& start_session,
        ICountApplications& count_applications,
        IFindPrimitiveOptions& find_primitive_options,
        IChooseStep& choose_step,
        IChoosePrimitive& choose_primitive,
        IApplyChosenPrimitive& apply_chosen_primitive,
        IGetFeatures& get_features,
        IFindOutput& find_output,
        size_t max_applications);
    const std::vector<candidate>& synthesize(const bin_view& data);
private:
    IStartSession& start_session_;
    ICountApplications& count_applications_;
    IFindPrimitiveOptions& find_primitive_options_;
    IChooseStep& choose_step_;
    IChoosePrimitive& choose_primitive_;
    IApplyChosenPrimitive& apply_chosen_primitive_;
    IGetFeatures& get_features_;
    IFindOutput& find_output_;
    size_t max_applications_;
};

#define FEATURE_SYNTHESIZER_TEMPLATE                                                                              \
    template<                                                                                                     \
        typename ISS, typename ICA, typename IFPO, typename ICS, typename ICP, typename IACP, typename IGF,       \
        typename IFO>
#define FEATURE_SYNTHESIZER feature_synthesizer<ISS, ICA, IFPO, ICS, ICP, IACP, IGF, IFO>

FEATURE_SYNTHESIZER_TEMPLATE
FEATURE_SYNTHESIZER::feature_synthesizer(
    ISS& start_session,
    ICA& count_applications,
    IFPO& find_primitive_options,
    ICS& choose_step,
    ICP& choose_primitive,
    IACP& apply_chosen_primitive,
    IGF& get_features,
    IFO& find_output,
    size_t max_applications)
    : start_session_(start_session)
    , count_applications_(count_applications)
    , find_primitive_options_(find_primitive_options)
    , choose_step_(choose_step)
    , choose_primitive_(choose_primitive)
    , apply_chosen_primitive_(apply_chosen_primitive)
    , get_features_(get_features)
    , find_output_(find_output)
    , max_applications_(max_applications) {}

FEATURE_SYNTHESIZER_TEMPLATE
const std::vector<candidate>& FEATURE_SYNTHESIZER::synthesize(const bin_view& data) {
    start_session_.start(data);
    while (count_applications_.count() < max_applications_) {
        if (find_output_.find(get_features_.candidates(), data.labels).has_value())
            break;
        const std::vector<prim_id> options = find_primitive_options_.find();
        if (options.empty())
            break;
        const bool stop = choose_step_.choose(2) == 0;
        if (stop)
            break;
        const size_t chosen = choose_primitive_.choose(options.size());
        apply_chosen_primitive_.apply(options.at(chosen), data.labels.rows->size());
    }
    return get_features_.candidates();
}

#undef FEATURE_SYNTHESIZER
#undef FEATURE_SYNTHESIZER_TEMPLATE

}

#endif
