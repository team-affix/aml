#ifndef KITE_OPTIONS_HPP
#define KITE_OPTIONS_HPP

#include <cstddef>
#include <cstdint>

namespace AML {

struct kite_options {
    size_t max_applications;
    double exploration_constant;
    uint32_t seed;
    auto operator<=>(const kite_options&) const = default;
};

}

#endif
