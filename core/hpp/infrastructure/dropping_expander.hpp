#ifndef DROPPING_EXPANDER_HPP
#define DROPPING_EXPANDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/feature_id.hpp"

namespace AML {

// Expands a set by drops: pushes one child per member the set can drop and still be a function, without that member.
template<typename IFindDroppableMembers, typename IPushChild>
struct dropping_expander {
    dropping_expander(IFindDroppableMembers& find_droppable_members, IPushChild& push_child);
    void expand(const std::vector<feature_id>& members);
private:
    IFindDroppableMembers& find_droppable_members_;
    IPushChild& push_child_;
};

#define DROPPING_EXPANDER_TEMPLATE template<typename IFindDroppableMembers, typename IPushChild>
#define DROPPING_EXPANDER          dropping_expander<IFindDroppableMembers, IPushChild>

DROPPING_EXPANDER_TEMPLATE
DROPPING_EXPANDER::dropping_expander(IFindDroppableMembers& find_droppable_members, IPushChild& push_child)
    : find_droppable_members_(find_droppable_members)
    , push_child_(push_child) {}

DROPPING_EXPANDER_TEMPLATE
void DROPPING_EXPANDER::expand(const std::vector<feature_id>& members) {
    for (const size_t position : find_droppable_members_.find(members)) {
        std::vector<feature_id> child = members;
        child.erase(child.begin() + static_cast<std::ptrdiff_t>(position));
        push_child_.push(std::move(child));
    }
}

#undef DROPPING_EXPANDER
#undef DROPPING_EXPANDER_TEMPLATE

}

#endif
