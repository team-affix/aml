#ifndef BINARY_NATURAL_DIV_HPP
#define BINARY_NATURAL_DIV_HPP

#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"
#include "value_objects/value.hpp"

// Integer division of two binary naturals, rounded down; dividing by zero gives zero.
struct binary_natural_div {
    binary_natural_div(binary_natural_ctors naturals);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    binary_natural_writer writer_;
};

#endif
