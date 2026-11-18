#ifndef OPTION_RANGE_HPP
#define OPTION_RANGE_HPP

#include <cstddef>
#include "value_objects/mcts_choice.hpp"

// The options 0..count-1, offered to MCTS by count and position without laying them out.
struct option_range {
    option_range(size_t count);
    size_t size() const;
    mcts_choice at(size_t index) const;
private:
    size_t count_;
};

#endif
