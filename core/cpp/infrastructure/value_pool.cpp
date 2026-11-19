#include "infrastructure/value_pool.hpp"

value_pool::value_pool()
    : values_() {}

const value* value_pool::make(size_t tag, const std::vector<const value*>& arguments) {
    return &*values_.insert(value{.tag = tag, .args = arguments}).first;
}
