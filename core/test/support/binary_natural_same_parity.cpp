#include "support/binary_natural_same_parity.hpp"
#include "debug_assert.hpp"

binary_natural_same_parity::binary_natural_same_parity(
    binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value binary_natural_same_parity::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool answer = reader_(*args[0]) % 2 == reader_(*args[1]) % 2;
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
