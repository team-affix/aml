#ifndef FEATURE_SYNTHESIZER_HPP
#define FEATURE_SYNTHESIZER_HPP

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/prim_application.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_signature.hpp"

// Synthesizes heterogeneous features of a bin in one session of at most `max_applications` primitive applications.
// Each step chooses to stop or to apply one of the primitives that has an application witness; then the arguments
// are chosen, the application is logged, and a heterogeneous result joins the features, cheapest per column. The
// session stops when the stop option is chosen, the cap is reached, or no primitive has a witness. Option 0 is stop.
// The features returned are the session's own: they hold until the next session starts.
template<
    typename IStartSession,
    typename ICountApplications,
    typename IFindPrimitiveOptions,
    typename IChooseStep,
    typename IGetPrimitiveSignature,
    typename IChooseArguments,
    typename IRecordApplication,
    typename IGetFeatureCandidates,
    typename IFindAppCandidate,
    typename ITryInsertFeatureCandidate>
struct feature_synthesizer {
    feature_synthesizer(
        IStartSession& start_session,
        ICountApplications& count_applications,
        IFindPrimitiveOptions& find_primitive_options,
        IChooseStep& choose_step,
        IGetPrimitiveSignature& get_primitive_signature,
        IChooseArguments& choose_arguments,
        IRecordApplication& record_application,
        IGetFeatureCandidates& get_feature_candidates,
        IFindAppCandidate& find_app_candidate,
        ITryInsertFeatureCandidate& try_insert_feature_candidate,
        size_t max_applications);
    const std::vector<candidate>& synthesize(const bin& data);
private:
    IStartSession& start_session_;
    ICountApplications& count_applications_;
    IFindPrimitiveOptions& find_primitive_options_;
    IChooseStep& choose_step_;
    IGetPrimitiveSignature& get_primitive_signature_;
    IChooseArguments& choose_arguments_;
    IRecordApplication& record_application_;
    IGetFeatureCandidates& get_feature_candidates_;
    IFindAppCandidate& find_app_candidate_;
    ITryInsertFeatureCandidate& try_insert_feature_candidate_;
    size_t max_applications_;
};

#define FEATURE_SYNTHESIZER_TEMPLATE                                                                              \
    template<typename ISS, typename ICA, typename IFPO, typename ICS, typename IGPS, typename ICAr, typename IRA, \
             typename IGFC, typename IFAC, typename ITIFC>
#define FEATURE_SYNTHESIZER feature_synthesizer<ISS, ICA, IFPO, ICS, IGPS, ICAr, IRA, IGFC, IFAC, ITIFC>

FEATURE_SYNTHESIZER_TEMPLATE
FEATURE_SYNTHESIZER::feature_synthesizer(
    ISS& start_session,
    ICA& count_applications,
    IFPO& find_primitive_options,
    ICS& choose_step,
    IGPS& get_primitive_signature,
    ICAr& choose_arguments,
    IRA& record_application,
    IGFC& get_feature_candidates,
    IFAC& find_app_candidate,
    ITIFC& try_insert_feature_candidate,
    size_t max_applications)
    : start_session_(start_session)
    , count_applications_(count_applications)
    , find_primitive_options_(find_primitive_options)
    , choose_step_(choose_step)
    , get_primitive_signature_(get_primitive_signature)
    , choose_arguments_(choose_arguments)
    , record_application_(record_application)
    , get_feature_candidates_(get_feature_candidates)
    , find_app_candidate_(find_app_candidate)
    , try_insert_feature_candidate_(try_insert_feature_candidate)
    , max_applications_(max_applications) {}

FEATURE_SYNTHESIZER_TEMPLATE
const std::vector<candidate>& FEATURE_SYNTHESIZER::synthesize(const bin& data) {
    start_session_.start(data);
    while (count_applications_.count() < max_applications_) {
        const std::vector<prim_id> options = find_primitive_options_.find();
        if (options.empty())
            break;
        const size_t step = choose_step_.choose(options.size() + 1);
        if (step == 0)
            break;
        const prim_id prim = options.at(step - 1);
        const primitive_signature signature = get_primitive_signature_.signature(prim);
        const std::vector<size_t> args = choose_arguments_.choose(prim, signature.arg_types);
        record_application_.record(prim_application{.prim = prim, .args = args});
        std::optional<candidate> application =
            find_app_candidate_.find(prim, signature.result_type, args, data.labels.size());
        if (!application.has_value())
            continue;
        try_insert_feature_candidate_.try_insert(std::move(application.value()));
    }
    return get_feature_candidates_.candidates();
}

#undef FEATURE_SYNTHESIZER
#undef FEATURE_SYNTHESIZER_TEMPLATE

#endif
