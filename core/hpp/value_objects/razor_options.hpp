#ifndef RAZOR_OPTIONS_HPP
#define RAZOR_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

struct razor_options {
    size_t max_applications;
    double exploration_constant;
    uint32_t seed;
    auto operator<=>(const razor_options&) const = default;
};

#endif
