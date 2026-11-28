#ifndef BINARY_NATURAL_ABSDIFF_HPP
#define BINARY_NATURAL_ABSDIFF_HPP

#include <vector>
#include "support/binary_natural_ctors.hpp"
#include "support/binary_natural_reader.hpp"
#include "support/binary_natural_writer.hpp"
#include "value_objects/value.hpp"

// The distance between two binary naturals, |a - b|.
struct binary_natural_absdiff {
    binary_natural_absdiff(binary_natural_ctors naturals);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    binary_natural_reader reader_;
    binary_natural_writer writer_;
};

#endif
