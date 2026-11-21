#ifndef BINARY_NATURAL_MUL_HPP
#define BINARY_NATURAL_MUL_HPP

#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"
#include "value_objects/value.hpp"

// Multiplication of two binary naturals.
struct binary_natural_mul {
    binary_natural_mul(binary_natural_ctors naturals);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    binary_natural_writer writer_;
};

#endif
