#include "support/binary_natural_add.hpp"
#include "debug_assert.hpp"

binary_natural_add::binary_natural_add(binary_natural_ctors naturals)
    : reader_(naturals)
    , writer_(naturals) {}

AML::value binary_natural_add::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const unsigned __int128 left = reader_(*args[0]);
    return writer_(left + reader_(*args[1]));
}
