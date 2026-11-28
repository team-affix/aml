#ifndef BINARY_NATURAL_SAME_RESIDUE_HPP
#define BINARY_NATURAL_SAME_RESIDUE_HPP

#include <cstddef>
#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Whether two binary naturals leave the same remainder modulo `modulus` (not zero), answered in a user boolean type
// with the constructors `false_ctor` and `true_ctor`.
struct binary_natural_same_residue {
    binary_natural_same_residue(
        binary_natural_ctors naturals, size_t modulus, AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    size_t modulus_;
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
