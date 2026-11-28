#include "support/binary_natural_mod.hpp"
#include "debug_assert.hpp"

binary_natural_mod::binary_natural_mod(binary_natural_ctors naturals)
    : reader_(naturals)
    , writer_(naturals) {}

AML::value binary_natural_mod::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const size_t dividend = reader_(*args[0]);
    const size_t divisor = reader_(*args[1]);
    return writer_(divisor == 0 ? dividend : dividend % divisor);
}
