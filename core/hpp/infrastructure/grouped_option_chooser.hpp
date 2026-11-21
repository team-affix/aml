#ifndef GROUPED_OPTION_CHOOSER_HPP
#define GROUPED_OPTION_CHOOSER_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>
#include "debug_assert.hpp"

namespace AML {

// Chooses an option by its key in two flat choices: first one group among the keys present, highest first, then one
// of that group's options, in their given order. A choice with one possibility is taken without asking, so one key
// makes no group choice and a group of one option makes no choice within it. Returns the chosen option's position in
// `keys`.
template<typename IChooseGroup, typename IChooseInGroup>
struct grouped_option_chooser {
    grouped_option_chooser(IChooseGroup& choose_group, IChooseInGroup& choose_in_group);
    size_t choose(const std::vector<size_t>& keys);
private:
    IChooseGroup& choose_group_;
    IChooseInGroup& choose_in_group_;
};

#define GROUPED_OPTION_CHOOSER_TEMPLATE template<typename IChooseGroup, typename IChooseInGroup>
#define GROUPED_OPTION_CHOOSER          grouped_option_chooser<IChooseGroup, IChooseInGroup>

GROUPED_OPTION_CHOOSER_TEMPLATE
GROUPED_OPTION_CHOOSER::grouped_option_chooser(IChooseGroup& choose_group, IChooseInGroup& choose_in_group)
    : choose_group_(choose_group)
    , choose_in_group_(choose_in_group) {}

GROUPED_OPTION_CHOOSER_TEMPLATE
size_t GROUPED_OPTION_CHOOSER::choose(const std::vector<size_t>& keys) {
    DEBUG_ASSERT(!keys.empty());
    std::vector<size_t> groups = keys;
    std::sort(groups.begin(), groups.end(), std::greater<size_t>());
    groups.erase(std::unique(groups.begin(), groups.end()), groups.end());
    const bool one_group = groups.size() == 1;
    const size_t group = one_group ? 0 : choose_group_.choose(groups.size());
    std::vector<size_t> members;
    for (size_t position = 0; position < keys.size(); ++position) {
        if (keys[position] != groups.at(group))
            continue;
        members.push_back(position);
    }
    if (members.size() == 1)
        return members.front();
    return members.at(choose_in_group_.choose(members.size()));
}

#undef GROUPED_OPTION_CHOOSER
#undef GROUPED_OPTION_CHOOSER_TEMPLATE

}

#endif
