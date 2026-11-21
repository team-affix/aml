#ifndef SWIFT_OPTIONS_HPP
#define SWIFT_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

// best_weight (w) blends a node's mean reward with its best: selection reads (1 - w) * mean + w * best. In rollouts,
// fresh_weight is the probability of taking a fresh operand over a consumed one and made_weight of taking a made one
// over a given one, when both are possible; 0.5 is no preference.
struct swift_options {
    size_t max_applications;
    double exploration_constant;
    double best_weight;
    double fresh_weight;
    double made_weight;
    uint32_t seed;
    auto operator<=>(const swift_options&) const = default;
};

}

#endif
