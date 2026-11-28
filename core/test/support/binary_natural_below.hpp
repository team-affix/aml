#ifndef BINARY_NATURAL_BELOW_HPP
#define BINARY_NATURAL_BELOW_HPP

#include <cstddef>
#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Whether a binary natural is less than `bound`, answered in a user boolean type with the constructors
// `false_ctor` and `true_ctor`.
struct binary_natural_below {
    binary_natural_below(binary_natural_ctors naturals, size_t bound, AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    size_t bound_;
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
