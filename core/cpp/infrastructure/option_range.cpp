#include "infrastructure/option_range.hpp"
#include "debug_assert.hpp"

option_range::option_range(size_t count)
    : count_(count) {}

size_t option_range::size() const {
    return count_;
}

mcts_choice option_range::at(size_t index) const {
    DEBUG_ASSERT(index < count_);
    return index;
}
