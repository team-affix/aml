#include "support/binary_natural_divisible_by.hpp"
#include "debug_assert.hpp"

binary_natural_divisible_by::binary_natural_divisible_by(
    binary_natural_ctors naturals, size_t divisor, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , divisor_(divisor)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {
    DEBUG_ASSERT(divisor_ != 0);
}

AML::value binary_natural_divisible_by::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool answer = reader_(*args[0]) % divisor_ == 0;
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
