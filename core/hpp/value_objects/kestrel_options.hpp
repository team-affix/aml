#ifndef KESTREL_OPTIONS_HPP
#define KESTREL_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

// best_weight (w) blends a node's mean reward with its best: selection reads (1 - w) * mean + w * best. With w = 0
// Kestrel searches as Kite does.
struct kestrel_options {
    size_t max_applications;
    double exploration_constant;
    double best_weight;
    uint32_t seed;
    auto operator<=>(const kestrel_options&) const = default;
};

}

#endif
