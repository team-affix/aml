#ifndef OSPREY_OPTIONS_HPP
#define OSPREY_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

struct osprey_options {
    size_t max_applications;
    double exploration_constant;
    uint32_t seed;
    size_t commit_visits;
    auto operator<=>(const osprey_options&) const = default;
};

}

#endif
