#ifndef CONSTANT_ANCESTRY_FINDER_HPP
#define CONSTANT_ANCESTRY_FINDER_HPP

#include <algorithm>
#include <unordered_set>
#include <variant>
#include <vector>
#include "value_objects/constant_derivation.hpp"
#include "value_objects/constant_id.hpp"
#include "value_objects/constant_record.hpp"

namespace AML {

// Finds the ancestry of some constants: the constants themselves and every constant their origins apply primitives
// to, transitively, each once, in increasing id.
template<typename IGetConstantRecord>
struct constant_ancestry_finder {
    constant_ancestry_finder(IGetConstantRecord& get_constant_record);
    std::vector<constant_id> find(const std::vector<constant_id>& roots) const;
private:
    IGetConstantRecord& get_constant_record_;
};

#define CONSTANT_ANCESTRY_FINDER_TEMPLATE template<typename IGetConstantRecord>
#define CONSTANT_ANCESTRY_FINDER          constant_ancestry_finder<IGetConstantRecord>

CONSTANT_ANCESTRY_FINDER_TEMPLATE
CONSTANT_ANCESTRY_FINDER::constant_ancestry_finder(IGetConstantRecord& get_constant_record)
    : get_constant_record_(get_constant_record) {}

CONSTANT_ANCESTRY_FINDER_TEMPLATE
std::vector<constant_id> CONSTANT_ANCESTRY_FINDER::find(const std::vector<constant_id>& roots) const {
    std::unordered_set<constant_id> found;
    std::vector<constant_id> pending(roots);
    while (!pending.empty()) {
        const constant_id constant = pending.back();
        pending.pop_back();
        if (!found.insert(constant).second)
            continue;
        const constant_derivation& origin = get_constant_record_.record(constant).origin;
        const constant_derivation::application* application =
            std::get_if<constant_derivation::application>(&origin.content);
        if (application == nullptr)
            continue;
        pending.insert(pending.end(), application->args.begin(), application->args.end());
    }
    std::vector<constant_id> ancestry(found.begin(), found.end());
    std::sort(ancestry.begin(), ancestry.end());
    return ancestry;
}

#undef CONSTANT_ANCESTRY_FINDER
#undef CONSTANT_ANCESTRY_FINDER_TEMPLATE

}

#endif
