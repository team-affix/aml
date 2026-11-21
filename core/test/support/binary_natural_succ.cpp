#include "support/binary_natural_succ.hpp"
#include "debug_assert.hpp"

binary_natural_succ::binary_natural_succ(binary_natural_ctors naturals)
    : reader_(naturals)
    , writer_(naturals) {}

AML::value binary_natural_succ::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const unsigned __int128 number = reader_(*args[0]);
    return writer_(number + 1);
}
