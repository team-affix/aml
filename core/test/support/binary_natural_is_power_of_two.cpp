#include "support/binary_natural_is_power_of_two.hpp"
#include <bit>
#include "debug_assert.hpp"

binary_natural_is_power_of_two::binary_natural_is_power_of_two(
    binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value binary_natural_is_power_of_two::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool answer = std::has_single_bit(reader_(*args[0]));
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
