#ifndef VALUE_POOL_HPP
#define VALUE_POOL_HPP

#include <cstddef>
#include <unordered_set>
#include <vector>
#include "value_objects/value.hpp"
#include "value_objects/value_hash.hpp"

// Interns values, so equal values share one pointer.
struct value_pool {
    value_pool();
    const value* make(size_t tag, const std::vector<const value*>& arguments);
private:
    std::unordered_set<value, value_hash> values_;
};

#endif
