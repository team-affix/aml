#include "support/binary_natural_pred.hpp"
#include "debug_assert.hpp"

binary_natural_pred::binary_natural_pred(binary_natural_ctors naturals)
    : reader_(naturals)
    , writer_(naturals) {}

AML::value binary_natural_pred::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const size_t number = reader_(*args[0]);
    return writer_(number > 0 ? number - 1 : 0);
}
