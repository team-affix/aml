#ifndef BINARY_NATURAL_WRITER_HPP
#define BINARY_NATURAL_WRITER_HPP

#include <cstddef>
#include "support/binary_natural_ctors.hpp"
#include "value_objects/value.hpp"

// Writes a number as a binary natural value, capped at k_binary_natural_largest so that repeated multiplying and
// squaring stay bounded.
const size_t k_binary_natural_largest = size_t{1} << 40;

struct binary_natural_writer {
    binary_natural_writer(binary_natural_ctors naturals);
    AML::value operator()(unsigned __int128 number) const;
private:
    binary_natural_ctors naturals_;
};

#endif
