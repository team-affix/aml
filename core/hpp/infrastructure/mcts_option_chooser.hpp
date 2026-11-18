#ifndef MCTS_OPTION_CHOOSER_HPP
#define MCTS_OPTION_CHOOSER_HPP

#include <cstddef>
#include "infrastructure/option_range.hpp"
#include "debug_assert.hpp"

// Chooses an option by asking MCTS to choose among the options' indices.
template<typename IChooseChoice>
struct mcts_option_chooser {
    mcts_option_chooser(IChooseChoice& choose_choice);
    size_t choose(size_t option_count);
private:
    IChooseChoice& choose_choice_;
};

#define MCTS_OPTION_CHOOSER_TEMPLATE template<typename IChooseChoice>
#define MCTS_OPTION_CHOOSER          mcts_option_chooser<IChooseChoice>

MCTS_OPTION_CHOOSER_TEMPLATE
MCTS_OPTION_CHOOSER::mcts_option_chooser(IChooseChoice& choose_choice)
    : choose_choice_(choose_choice) {}

MCTS_OPTION_CHOOSER_TEMPLATE
size_t MCTS_OPTION_CHOOSER::choose(size_t option_count) {
    DEBUG_ASSERT(option_count > 0);
    const option_range options(option_count);
    return choose_choice_.choose(options, options);
}

#undef MCTS_OPTION_CHOOSER
#undef MCTS_OPTION_CHOOSER_TEMPLATE

#endif
