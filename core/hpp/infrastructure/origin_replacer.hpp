#ifndef ORIGIN_REPLACER_HPP
#define ORIGIN_REPLACER_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/constant_id.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/operand.hpp"

namespace AML {

// Replaces a feature's origin with an application making the same column when the application's ancestry, across
// both graphs, is smaller. An application built on the feature itself has the feature's whole ancestry and more, so it
// never replaces it and the graph stays acyclic.
template<typename ISizeAncestry, typename IReplaceOrigin>
struct origin_replacer {
    origin_replacer(ISizeAncestry& size_ancestry, IReplaceOrigin& replace_origin);
    void try_replace(feature_id feature, const derivation& candidate);
private:
    ISizeAncestry& size_ancestry_;
    IReplaceOrigin& replace_origin_;
};

#define ORIGIN_REPLACER_TEMPLATE template<typename ISizeAncestry, typename IReplaceOrigin>
#define ORIGIN_REPLACER          origin_replacer<ISizeAncestry, IReplaceOrigin>

ORIGIN_REPLACER_TEMPLATE
ORIGIN_REPLACER::origin_replacer(ISizeAncestry& size_ancestry, IReplaceOrigin& replace_origin)
    : size_ancestry_(size_ancestry)
    , replace_origin_(replace_origin) {}

ORIGIN_REPLACER_TEMPLATE
void ORIGIN_REPLACER::try_replace(feature_id feature, const derivation& candidate) {
    std::vector<feature_id> features;
    std::vector<constant_id> constants;
    for (const operand& arg : std::get<derivation::application>(candidate.content).args) {
        const operand::from_feature* argument = std::get_if<operand::from_feature>(&arg.source);
        if (argument != nullptr) {
            features.push_back(argument->id);
            continue;
        }
        constants.push_back(std::get<operand::from_constant>(arg.source).id);
    }
    const size_t candidate_size = 1 + size_ancestry_.size(features, constants);
    const size_t held_size = size_ancestry_.size({feature}, {});
    if (candidate_size >= held_size)
        return;
    replace_origin_.replace_origin(feature, candidate);
}

#undef ORIGIN_REPLACER
#undef ORIGIN_REPLACER_TEMPLATE

}

#endif
