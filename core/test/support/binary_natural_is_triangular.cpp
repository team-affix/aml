#include "support/binary_natural_is_triangular.hpp"
#include <cmath>
#include "debug_assert.hpp"

namespace {

// The largest root whose square is at most `number`.
size_t integer_square_root(unsigned __int128 number) {
    size_t root = static_cast<size_t>(std::sqrt(static_cast<long double>(number)));
    while (static_cast<unsigned __int128>(root) * root > number)
        --root;
    while (static_cast<unsigned __int128>(root + 1) * (root + 1) <= number)
        ++root;
    return root;
}

}

binary_natural_is_triangular::binary_natural_is_triangular(
    binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value binary_natural_is_triangular::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    // k (k + 1) / 2 = n gives k = floor(sqrt(2n)).
    const size_t number = reader_(*args[0]);
    const size_t root = integer_square_root(static_cast<unsigned __int128>(number) * 2);
    const bool answer = static_cast<unsigned __int128>(root) * (root + 1) / 2 == number;
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
