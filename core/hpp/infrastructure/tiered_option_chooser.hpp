#ifndef TIERED_OPTION_CHOOSER_HPP
#define TIERED_OPTION_CHOOSER_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>
#include "debug_assert.hpp"

namespace AML {

// Chooses an option by its order, highest first. Starting from the highest order present, a two-way choice takes
// that order (option 0) or descends to the next lower order present (option 1); the lowest order is taken without a
// choice. One of the taken order's options is then chosen, in their given order. Returns the chosen option's position
// in `orders`.
template<typename IChooseTier, typename IChooseInTier>
struct tiered_option_chooser {
    tiered_option_chooser(IChooseTier& choose_tier, IChooseInTier& choose_in_tier);
    size_t choose(const std::vector<size_t>& orders);
private:
    IChooseTier& choose_tier_;
    IChooseInTier& choose_in_tier_;
};

#define TIERED_OPTION_CHOOSER_TEMPLATE template<typename IChooseTier, typename IChooseInTier>
#define TIERED_OPTION_CHOOSER          tiered_option_chooser<IChooseTier, IChooseInTier>

TIERED_OPTION_CHOOSER_TEMPLATE
TIERED_OPTION_CHOOSER::tiered_option_chooser(IChooseTier& choose_tier, IChooseInTier& choose_in_tier)
    : choose_tier_(choose_tier)
    , choose_in_tier_(choose_in_tier) {}

TIERED_OPTION_CHOOSER_TEMPLATE
size_t TIERED_OPTION_CHOOSER::choose(const std::vector<size_t>& orders) {
    DEBUG_ASSERT(!orders.empty());
    std::vector<size_t> tiers = orders;
    std::sort(tiers.begin(), tiers.end(), std::greater<size_t>());
    tiers.erase(std::unique(tiers.begin(), tiers.end()), tiers.end());
    size_t level = 0;
    while (level + 1 < tiers.size() && choose_tier_.choose(2) == 1)
        ++level;
    std::vector<size_t> members;
    for (size_t position = 0; position < orders.size(); ++position) {
        if (orders[position] != tiers[level])
            continue;
        members.push_back(position);
    }
    return members.at(choose_in_tier_.choose(members.size()));
}

#undef TIERED_OPTION_CHOOSER
#undef TIERED_OPTION_CHOOSER_TEMPLATE

}

#endif
