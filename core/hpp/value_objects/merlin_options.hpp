#ifndef MERLIN_OPTIONS_HPP
#define MERLIN_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

// best_weight (w) blends a node's mean reward with its best: selection reads (1 - w) * mean + w * best.
struct merlin_options {
    size_t max_applications;
    double exploration_constant;
    double best_weight;
    uint32_t seed;
    auto operator<=>(const merlin_options&) const = default;
};

}

#endif
