#ifndef HERON_OPTIONS_HPP
#define HERON_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

// focus_width (A), focus_rate (r) and focus_bonus (beta) are on the normalized [0, 1] scale of the means: the band
// is A * N^(-r) for a node with N visits.
struct heron_options {
    size_t max_applications;
    double exploration_constant;
    double focus_width;
    double focus_rate;
    double focus_bonus;
    uint32_t seed;
    auto operator<=>(const heron_options&) const = default;
};

}

#endif
