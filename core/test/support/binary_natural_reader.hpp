#ifndef BINARY_NATURAL_READER_HPP
#define BINARY_NATURAL_READER_HPP

#include <cstddef>
#include "support/binary_natural_ctors.hpp"
#include "value_objects/value.hpp"

// Reads the number a binary natural value stands for.
struct binary_natural_reader {
    binary_natural_reader(binary_natural_ctors naturals);
    size_t operator()(const AML::value& term) const;
private:
    binary_natural_ctors naturals_;
};

#endif
