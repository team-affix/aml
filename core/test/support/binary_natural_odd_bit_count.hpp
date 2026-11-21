#ifndef BINARY_NATURAL_ODD_BIT_COUNT_HPP
#define BINARY_NATURAL_ODD_BIT_COUNT_HPP

#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Whether a binary natural has an odd number of 1 bits, answered in a user boolean type with the constructors
// `false_ctor` and `true_ctor`.
struct binary_natural_odd_bit_count {
    binary_natural_odd_bit_count(binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
