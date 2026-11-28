#ifndef ADDING_EXPANDER_HPP
#define ADDING_EXPANDER_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Expands a set by additions: samples up to adds_per_feature applications per feature in the set's ancestry, each over
// the ancestry's features and the constants with at least one feature among its operands, and pushes the set with
// each derived feature it does not hold yet. A sample with no application, or an application deriving no feature, is
// spent without a child.
template<
    typename IFindAncestry, typename IFindOperandPool, typename IFindEligiblePrimitives, typename ISampleDerivation,
    typename IDeriveFeature, typename IPushChild>
struct adding_expander {
    adding_expander(
        IFindAncestry& find_ancestry,
        IFindOperandPool& find_operand_pool,
        IFindEligiblePrimitives& find_eligible_primitives,
        ISampleDerivation& sample_derivation,
        IDeriveFeature& derive_feature,
        IPushChild& push_child,
        size_t adds_per_feature);
    void expand(const std::vector<feature_id>& members);
private:
    IFindAncestry& find_ancestry_;
    IFindOperandPool& find_operand_pool_;
    IFindEligiblePrimitives& find_eligible_primitives_;
    ISampleDerivation& sample_derivation_;
    IDeriveFeature& derive_feature_;
    IPushChild& push_child_;
    size_t adds_per_feature_;
};

#define ADDING_EXPANDER_TEMPLATE \
    template< \
        typename IFindAncestry, typename IFindOperandPool, typename IFindEligiblePrimitives, \
        typename ISampleDerivation, typename IDeriveFeature, typename IPushChild>
#define ADDING_EXPANDER \
    adding_expander< \
        IFindAncestry, IFindOperandPool, IFindEligiblePrimitives, ISampleDerivation, IDeriveFeature, IPushChild>

ADDING_EXPANDER_TEMPLATE
ADDING_EXPANDER::adding_expander(
    IFindAncestry& find_ancestry,
    IFindOperandPool& find_operand_pool,
    IFindEligiblePrimitives& find_eligible_primitives,
    ISampleDerivation& sample_derivation,
    IDeriveFeature& derive_feature,
    IPushChild& push_child,
    size_t adds_per_feature)
    : find_ancestry_(find_ancestry)
    , find_operand_pool_(find_operand_pool)
    , find_eligible_primitives_(find_eligible_primitives)
    , sample_derivation_(sample_derivation)
    , derive_feature_(derive_feature)
    , push_child_(push_child)
    , adds_per_feature_(adds_per_feature) {}

ADDING_EXPANDER_TEMPLATE
void ADDING_EXPANDER::expand(const std::vector<feature_id>& members) {
    const std::vector<feature_id> ancestry = find_ancestry_.find(members);
    const std::unordered_map<std::type_index, std::vector<feature_id>> pool = find_operand_pool_.find(ancestry);
    const std::vector<prim_id> eligible = find_eligible_primitives_.find(pool);
    const size_t sample_count = adds_per_feature_ * ancestry.size();
    for (size_t sample = 0; sample < sample_count; ++sample) {
        const std::optional<derivation> origin = sample_derivation_.sample(pool, eligible);
        if (!origin.has_value())
            continue;
        const std::optional<feature_id> derived = derive_feature_.derive(origin.value());
        if (!derived.has_value())
            continue;
        const bool held = std::find(members.begin(), members.end(), derived.value()) != members.end();
        if (held)
            continue;
        std::vector<feature_id> child = members;
        child.push_back(derived.value());
        push_child_.push(std::move(child));
    }
}

#undef ADDING_EXPANDER
#undef ADDING_EXPANDER_TEMPLATE

}

#endif
