#ifndef ARGUMENT_TYPE_INDEX_HPP
#define ARGUMENT_TYPE_INDEX_HPP

#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/native_primitive.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Stores, for each type, the primitives taking an argument of it, each once, in increasing id.
struct argument_type_index {
    argument_type_index(const std::vector<native_primitive>& primitives);
    const std::vector<prim_id>& of_type(std::type_index type) const;
private:
    std::unordered_map<std::type_index, std::vector<prim_id>> by_type_;
    std::vector<prim_id> none_;
};

}

#endif
