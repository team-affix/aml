#include "support/binary_natural_same_residue.hpp"
#include "debug_assert.hpp"

binary_natural_same_residue::binary_natural_same_residue(
    binary_natural_ctors naturals, size_t modulus, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , modulus_(modulus)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {
    DEBUG_ASSERT(modulus_ != 0);
}

AML::value binary_natural_same_residue::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool answer = reader_(*args[0]) % modulus_ == reader_(*args[1]) % modulus_;
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
