#include "support/binary_natural_is_prime.hpp"
#include <array>
#include "debug_assert.hpp"

namespace {

// These bases decide primality exactly for every number below 3.3 * 10^24.
const std::array<size_t, 12> k_bases{2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

size_t power_mod(size_t base, size_t exponent, size_t modulus) {
    unsigned __int128 result = 1;
    unsigned __int128 square = base % modulus;
    for (; exponent > 0; exponent /= 2) {
        const bool odd = exponent % 2 == 1;
        if (odd)
            result = result * square % modulus;
        square = square * square % modulus;
    }
    return static_cast<size_t>(result);
}

// Whether `number` (odd, above every base) passes the strong probable prime test to `base`, where
// number - 1 = odd_part * 2^twos.
bool passes(size_t number, size_t base, size_t odd_part, size_t twos) {
    size_t residue = power_mod(base, odd_part, number);
    if (residue == 1 || residue == number - 1)
        return true;
    for (size_t step = 1; step < twos; ++step) {
        residue = static_cast<size_t>(static_cast<unsigned __int128>(residue) * residue % number);
        if (residue == number - 1)
            return true;
    }
    return false;
}

bool is_prime(size_t number) {
    if (number < 2)
        return false;
    for (const size_t base : k_bases) {
        if (number % base == 0)
            return number == base;
    }
    size_t odd_part = number - 1;
    size_t twos = 0;
    for (; odd_part % 2 == 0; odd_part /= 2)
        ++twos;
    for (const size_t base : k_bases) {
        if (!passes(number, base, odd_part, twos))
            return false;
    }
    return true;
}

}

binary_natural_is_prime::binary_natural_is_prime(
    binary_natural_ctors naturals, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : reader_(naturals)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value binary_natural_is_prime::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool answer = is_prime(reader_(*args[0]));
    return AML::value{.ctor = answer ? true_ctor_ : false_ctor_, .args = {}};
}
