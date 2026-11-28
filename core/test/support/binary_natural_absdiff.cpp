#include "support/binary_natural_absdiff.hpp"
#include "debug_assert.hpp"

binary_natural_absdiff::binary_natural_absdiff(binary_natural_ctors naturals)
    : reader_(naturals)
    , writer_(naturals) {}

AML::value binary_natural_absdiff::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const size_t left = reader_(*args[0]);
    const size_t right = reader_(*args[1]);
    return writer_(left > right ? left - right : right - left);
}
