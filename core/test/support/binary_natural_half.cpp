#include "support/binary_natural_half.hpp"
#include "debug_assert.hpp"

binary_natural_half::binary_natural_half(binary_natural_ctors naturals)
    : reader_(naturals)
    , writer_(naturals) {}

AML::value binary_natural_half::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    return writer_(reader_(*args[0]) / 2);
}
