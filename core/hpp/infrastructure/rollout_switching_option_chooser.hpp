#ifndef ROLLOUT_SWITCHING_OPTION_CHOOSER_HPP
#define ROLLOUT_SWITCHING_OPTION_CHOOSER_HPP

#include <cstddef>
#include <vector>

namespace AML {

// Chooses among options with keys one way in the search tree and another in a rollout, so the tree can make choices
// whose statistics mean something while random completions follow a different rule. Returns the chosen option's
// position in `keys`.
template<typename IIsInRollout, typename IChooseInTree, typename IChooseInRollout>
struct rollout_switching_option_chooser {
    rollout_switching_option_chooser(
        IIsInRollout& is_in_rollout, IChooseInTree& choose_in_tree, IChooseInRollout& choose_in_rollout);
    size_t choose(const std::vector<size_t>& keys);
private:
    IIsInRollout& is_in_rollout_;
    IChooseInTree& choose_in_tree_;
    IChooseInRollout& choose_in_rollout_;
};

#define ROLLOUT_SWITCHING_OPTION_CHOOSER_TEMPLATE \
    template<typename IIsInRollout, typename IChooseInTree, typename IChooseInRollout>
#define ROLLOUT_SWITCHING_OPTION_CHOOSER \
    rollout_switching_option_chooser<IIsInRollout, IChooseInTree, IChooseInRollout>

ROLLOUT_SWITCHING_OPTION_CHOOSER_TEMPLATE
ROLLOUT_SWITCHING_OPTION_CHOOSER::rollout_switching_option_chooser(
    IIsInRollout& is_in_rollout, IChooseInTree& choose_in_tree, IChooseInRollout& choose_in_rollout)
    : is_in_rollout_(is_in_rollout)
    , choose_in_tree_(choose_in_tree)
    , choose_in_rollout_(choose_in_rollout) {}

ROLLOUT_SWITCHING_OPTION_CHOOSER_TEMPLATE
size_t ROLLOUT_SWITCHING_OPTION_CHOOSER::choose(const std::vector<size_t>& keys) {
    if (is_in_rollout_.is_in_rollout())
        return choose_in_rollout_.choose(keys);
    return choose_in_tree_.choose(keys);
}

#undef ROLLOUT_SWITCHING_OPTION_CHOOSER
#undef ROLLOUT_SWITCHING_OPTION_CHOOSER_TEMPLATE

}

#endif
