#ifndef FALCON_OPTIONS_HPP
#define FALCON_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

struct falcon_options {
    size_t max_applications;
    double exploration_constant;
    uint32_t seed;
    size_t commit_visits;
    auto operator<=>(const falcon_options&) const = default;
};

#endif
