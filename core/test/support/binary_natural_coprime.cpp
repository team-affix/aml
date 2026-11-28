#include "support/binary_natural_coprime.hpp"
#include <numeric>
#include "debug_assert.hpp"

binary_natural_coprime::binary_natural_coprime(
    binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value binary_natural_coprime::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool answer = std::gcd(reader_(*args[0]), reader_(*args[1])) == 1;
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
