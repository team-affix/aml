#include "infrastructure/value_pool.hpp"

value_pool::value_pool()
    : values_() {}

const value* value_pool::make(ctor_id ctor, const std::vector<const value*>& fields) {
    return &*values_.insert(value{.ctor = ctor, .fields = fields}).first;
}
