#ifndef ROLLOUT_TIERED_OPTION_CHOOSER_HPP
#define ROLLOUT_TIERED_OPTION_CHOOSER_HPP

#include <cstddef>
#include <vector>

namespace AML {

// Chooses among options with orders. In the search tree, it is one choice among all of them, so the tree's statistics
// are per option. In a rollout, it chooses by order tiers (the highest order present half the time, else the next,
// and so on), so random completions build on high-order features. Returns the chosen option's position in `orders`.
template<typename IIsInRollout, typename IChooseFlat, typename IChooseByTier>
struct rollout_tiered_option_chooser {
    rollout_tiered_option_chooser(
        IIsInRollout& is_in_rollout, IChooseFlat& choose_flat, IChooseByTier& choose_by_tier);
    size_t choose(const std::vector<size_t>& orders);
private:
    IIsInRollout& is_in_rollout_;
    IChooseFlat& choose_flat_;
    IChooseByTier& choose_by_tier_;
};

#define ROLLOUT_TIERED_OPTION_CHOOSER_TEMPLATE \
    template<typename IIsInRollout, typename IChooseFlat, typename IChooseByTier>
#define ROLLOUT_TIERED_OPTION_CHOOSER          rollout_tiered_option_chooser<IIsInRollout, IChooseFlat, IChooseByTier>

ROLLOUT_TIERED_OPTION_CHOOSER_TEMPLATE
ROLLOUT_TIERED_OPTION_CHOOSER::rollout_tiered_option_chooser(
    IIsInRollout& is_in_rollout, IChooseFlat& choose_flat, IChooseByTier& choose_by_tier)
    : is_in_rollout_(is_in_rollout)
    , choose_flat_(choose_flat)
    , choose_by_tier_(choose_by_tier) {}

ROLLOUT_TIERED_OPTION_CHOOSER_TEMPLATE
size_t ROLLOUT_TIERED_OPTION_CHOOSER::choose(const std::vector<size_t>& orders) {
    if (is_in_rollout_.is_in_rollout())
        return choose_by_tier_.choose(orders);
    return choose_flat_.choose(orders.size());
}

#undef ROLLOUT_TIERED_OPTION_CHOOSER
#undef ROLLOUT_TIERED_OPTION_CHOOSER_TEMPLATE

}

#endif
