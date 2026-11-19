#include "infrastructure/value_pool.hpp"

value_pool::value_pool()
    : values_() {}

const value* value_pool::make(ctor_ref ctor, const std::vector<const value*>& arguments) {
    return &*values_.insert(value{.ctor = ctor, .args = arguments}).first;
}
