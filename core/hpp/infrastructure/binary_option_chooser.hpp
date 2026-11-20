#ifndef BINARY_OPTION_CHOOSER_HPP
#define BINARY_OPTION_CHOOSER_HPP

#include <cstddef>
#include "debug_assert.hpp"

namespace AML {

// Chooses among any number of options through two-way choices alone, so MCTS's branching is at most two everywhere.
// The options' indices are halved until one is left: each two-way choice keeps the lower half (option 0) or the upper
// half (option 1), the lower half taking the extra index of an odd range. A single option is chosen without asking.
template<typename IChooseHalf>
struct binary_option_chooser {
    binary_option_chooser(IChooseHalf& choose_half);
    size_t choose(size_t option_count);
private:
    IChooseHalf& choose_half_;
};

#define BINARY_OPTION_CHOOSER_TEMPLATE template<typename IChooseHalf>
#define BINARY_OPTION_CHOOSER          binary_option_chooser<IChooseHalf>

BINARY_OPTION_CHOOSER_TEMPLATE
BINARY_OPTION_CHOOSER::binary_option_chooser(IChooseHalf& choose_half)
    : choose_half_(choose_half) {}

BINARY_OPTION_CHOOSER_TEMPLATE
size_t BINARY_OPTION_CHOOSER::choose(size_t option_count) {
    DEBUG_ASSERT(option_count > 0);
    size_t low = 0;
    size_t high = option_count;
    while (high - low > 1) {
        const size_t middle = low + (high - low + 1) / 2;
        const bool upper = choose_half_.choose(2) == 1;
        if (upper) {
            low = middle;
            continue;
        }
        high = middle;
    }
    return low;
}

#undef BINARY_OPTION_CHOOSER
#undef BINARY_OPTION_CHOOSER_TEMPLATE

}

#endif
