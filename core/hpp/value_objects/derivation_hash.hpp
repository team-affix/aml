#ifndef DERIVATION_HASH_HPP
#define DERIVATION_HASH_HPP

#include <cstddef>
#include "value_objects/derivation.hpp"

namespace AML {

// Hashes a derivation by its kind and contents: an input's index, or an application's primitive and its operands in
// order, each by its kind and id.
struct derivation_hash {
    size_t operator()(const derivation& origin) const noexcept;
private:
    static size_t hash_combine(size_t seed, size_t hashed) noexcept;
};

}

#endif
