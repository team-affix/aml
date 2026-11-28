#ifndef ANCESTRY_FINDER_HPP
#define ANCESTRY_FINDER_HPP

#include <algorithm>
#include <unordered_set>
#include <variant>
#include <vector>
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"
#include "value_objects/operand.hpp"

namespace AML {

// Finds the feature ancestry of some features: the features themselves and every feature their origins apply
// primitives to, transitively, each once, in increasing id. Constant operands are not followed.
template<typename IGetFeatureRecord>
struct ancestry_finder {
    ancestry_finder(IGetFeatureRecord& get_feature_record);
    std::vector<feature_id> find(const std::vector<feature_id>& roots) const;
private:
    IGetFeatureRecord& get_feature_record_;
};

#define ANCESTRY_FINDER_TEMPLATE template<typename IGetFeatureRecord>
#define ANCESTRY_FINDER          ancestry_finder<IGetFeatureRecord>

ANCESTRY_FINDER_TEMPLATE
ANCESTRY_FINDER::ancestry_finder(IGetFeatureRecord& get_feature_record)
    : get_feature_record_(get_feature_record) {}

ANCESTRY_FINDER_TEMPLATE
std::vector<feature_id> ANCESTRY_FINDER::find(const std::vector<feature_id>& roots) const {
    std::unordered_set<feature_id> found;
    std::vector<feature_id> pending(roots);
    while (!pending.empty()) {
        const feature_id feature = pending.back();
        pending.pop_back();
        if (!found.insert(feature).second)
            continue;
        const derivation& origin = get_feature_record_.record(feature).origin;
        const derivation::application* application = std::get_if<derivation::application>(&origin.content);
        if (application == nullptr)
            continue;
        for (const operand& arg : application->args) {
            const operand::from_feature* argument = std::get_if<operand::from_feature>(&arg.source);
            if (argument == nullptr)
                continue;
            pending.push_back(argument->id);
        }
    }
    std::vector<feature_id> ancestry(found.begin(), found.end());
    std::sort(ancestry.begin(), ancestry.end());
    return ancestry;
}

#undef ANCESTRY_FINDER
#undef ANCESTRY_FINDER_TEMPLATE

}

#endif
