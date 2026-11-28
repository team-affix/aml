#ifndef ANCESTRY_SIZER_HPP
#define ANCESTRY_SIZER_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/constant_id.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"
#include "value_objects/operand.hpp"

namespace AML {

// Sizes the ancestry of some features and constants across both graphs: one per feature in the features' ancestry,
// plus one per constant in the ancestry of the given constants and of every constant those features apply
// primitives to. Shared ancestors count once.
template<typename IFindAncestry, typename IGetFeatureRecord, typename IFindConstantAncestry>
struct ancestry_sizer {
    ancestry_sizer(
        IFindAncestry& find_ancestry,
        IGetFeatureRecord& get_feature_record,
        IFindConstantAncestry& find_constant_ancestry);
    size_t size(const std::vector<feature_id>& features, const std::vector<constant_id>& constants) const;
private:
    IFindAncestry& find_ancestry_;
    IGetFeatureRecord& get_feature_record_;
    IFindConstantAncestry& find_constant_ancestry_;
};

#define ANCESTRY_SIZER_TEMPLATE \
    template<typename IFindAncestry, typename IGetFeatureRecord, typename IFindConstantAncestry>
#define ANCESTRY_SIZER ancestry_sizer<IFindAncestry, IGetFeatureRecord, IFindConstantAncestry>

ANCESTRY_SIZER_TEMPLATE
ANCESTRY_SIZER::ancestry_sizer(
    IFindAncestry& find_ancestry,
    IGetFeatureRecord& get_feature_record,
    IFindConstantAncestry& find_constant_ancestry)
    : find_ancestry_(find_ancestry)
    , get_feature_record_(get_feature_record)
    , find_constant_ancestry_(find_constant_ancestry) {}

ANCESTRY_SIZER_TEMPLATE
size_t ANCESTRY_SIZER::size(const std::vector<feature_id>& features, const std::vector<constant_id>& constants) const {
    const std::vector<feature_id> ancestry = find_ancestry_.find(features);
    std::vector<constant_id> constant_roots = constants;
    for (const feature_id feature : ancestry) {
        const derivation& origin = get_feature_record_.record(feature).origin;
        const derivation::application* application = std::get_if<derivation::application>(&origin.content);
        if (application == nullptr)
            continue;
        for (const operand& arg : application->args) {
            const operand::from_constant* constant = std::get_if<operand::from_constant>(&arg.source);
            if (constant == nullptr)
                continue;
            constant_roots.push_back(constant->id);
        }
    }
    return ancestry.size() + find_constant_ancestry_.find(constant_roots).size();
}

#undef ANCESTRY_SIZER
#undef ANCESTRY_SIZER_TEMPLATE

}

#endif
