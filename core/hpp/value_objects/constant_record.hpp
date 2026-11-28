#ifndef CONSTANT_RECORD_HPP
#define CONSTANT_RECORD_HPP

#include <compare>
#include <typeindex>
#include "value_objects/constant_derivation.hpp"

namespace AML {

// A constant of a constant graph: how it is made and its type. Its value lives in the constant cache.
struct constant_record {
    constant_derivation origin;
    std::type_index type;
    std::strong_ordering operator<=>(const constant_record&) const = default;
};

}

#endif
