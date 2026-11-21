#ifndef BINARY_NATURAL_IS_POWER_OF_TWO_HPP
#define BINARY_NATURAL_IS_POWER_OF_TWO_HPP

#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Whether a binary natural is a power of two (1, 2, 4, 8, ...), answered in a user boolean type with the constructors
// `false_ctor` and `true_ctor`.
struct binary_natural_is_power_of_two {
    binary_natural_is_power_of_two(binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
