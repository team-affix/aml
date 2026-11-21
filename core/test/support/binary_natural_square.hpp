#ifndef BINARY_NATURAL_SQUARE_HPP
#define BINARY_NATURAL_SQUARE_HPP

#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"
#include "value_objects/value.hpp"

// The square of a binary natural.
struct binary_natural_square {
    binary_natural_square(binary_natural_ctors naturals);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    binary_natural_writer writer_;
};

#endif
