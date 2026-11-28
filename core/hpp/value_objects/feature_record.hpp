#ifndef FEATURE_RECORD_HPP
#define FEATURE_RECORD_HPP

#include <cstdint>
#include <typeindex>
#include "value_objects/derivation.hpp"

namespace AML {

// A feature of a feature graph: how it is made, the type of its values, and the random key that identifies it in a
// set's key.
struct feature_record {
    derivation origin;
    std::type_index type;
    uint64_t key;
    std::strong_ordering operator<=>(const feature_record&) const = default;
};

}

#endif
