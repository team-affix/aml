#ifndef PRIMITIVE_APPLIER_HPP
#define PRIMITIVE_APPLIER_HPP

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/prim_application.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/primitive_signature.hpp"

// Applies one chosen primitive within a synthesis session: chooses its arguments, logs the application, and adds a
// heterogeneous result to the features.
template<
    typename IGetPrimitiveSignature,
    typename IChooseArguments,
    typename IRecordApplication,
    typename IFindAppCandidate,
    typename ITryInsertFeature>
struct primitive_applier {
    primitive_applier(
        IGetPrimitiveSignature& get_primitive_signature,
        IChooseArguments& choose_arguments,
        IRecordApplication& record_application,
        IFindAppCandidate& find_app_candidate,
        ITryInsertFeature& try_insert_feature);
    void apply(prim_id prim, size_t row_count);
private:
    IGetPrimitiveSignature& get_primitive_signature_;
    IChooseArguments& choose_arguments_;
    IRecordApplication& record_application_;
    IFindAppCandidate& find_app_candidate_;
    ITryInsertFeature& try_insert_feature_;
};

#define PRIMITIVE_APPLIER_TEMPLATE template<typename IGPS, typename ICA, typename IRA, typename IFAC, typename ITIF>
#define PRIMITIVE_APPLIER          primitive_applier<IGPS, ICA, IRA, IFAC, ITIF>

PRIMITIVE_APPLIER_TEMPLATE
PRIMITIVE_APPLIER::primitive_applier(
    IGPS& get_primitive_signature,
    ICA& choose_arguments,
    IRA& record_application,
    IFAC& find_app_candidate,
    ITIF& try_insert_feature)
    : get_primitive_signature_(get_primitive_signature)
    , choose_arguments_(choose_arguments)
    , record_application_(record_application)
    , find_app_candidate_(find_app_candidate)
    , try_insert_feature_(try_insert_feature) {}

PRIMITIVE_APPLIER_TEMPLATE
void PRIMITIVE_APPLIER::apply(prim_id prim, size_t row_count) {
    const primitive_signature signature = get_primitive_signature_.signature(prim);
    const std::vector<size_t> args = choose_arguments_.choose(prim, signature.arg_types);
    record_application_.record(prim_application{.prim = prim, .args = args});
    std::optional<candidate> application = find_app_candidate_.find(prim, signature.result_type, args, row_count);
    if (!application.has_value())
        return;
    try_insert_feature_.try_insert(std::move(application.value()));
}

#undef PRIMITIVE_APPLIER
#undef PRIMITIVE_APPLIER_TEMPLATE

#endif
