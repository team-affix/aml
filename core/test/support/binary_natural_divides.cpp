#include "support/binary_natural_divides.hpp"
#include "debug_assert.hpp"

binary_natural_divides::binary_natural_divides(
    binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value binary_natural_divides::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const size_t divisor = reader_(*args[0]);
    const size_t dividend = reader_(*args[1]);
    const bool answer = divisor == 0 ? dividend == 0 : dividend % divisor == 0;
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
