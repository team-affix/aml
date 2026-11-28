#include "infrastructure/argument_type_index.hpp"

namespace AML {

argument_type_index::argument_type_index(const std::vector<native_primitive>& primitives)
    : by_type_()
    , none_() {
    for (prim_id prim = 0; prim < primitives.size(); ++prim) {
        for (const std::type_index& type : primitives[prim].arg_types) {
            std::vector<prim_id>& takers = by_type_[type];
            const bool listed = !takers.empty() && takers.back() == prim;
            if (listed)
                continue;
            takers.push_back(prim);
        }
    }
}

const std::vector<prim_id>& argument_type_index::of_type(std::type_index type) const {
    const auto found = by_type_.find(type);
    if (found == by_type_.end())
        return none_;
    return found->second;
}

}
