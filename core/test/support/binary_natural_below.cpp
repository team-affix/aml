#include "support/binary_natural_below.hpp"
#include "debug_assert.hpp"

binary_natural_below::binary_natural_below(
    binary_natural_ctors naturals, size_t bound, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , bound_(bound)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value binary_natural_below::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool answer = reader_(*args[0]) < bound_;
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
