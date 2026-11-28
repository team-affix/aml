#ifndef NATIVE_TYPE_STORE_HPP
#define NATIVE_TYPE_STORE_HPP

#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/native_type.hpp"

namespace AML {

// Stores the native types a column solver was given, by their C++ type.
struct native_type_store {
    native_type_store(const std::vector<native_type>& types);
    const native_type& type(std::type_index type) const;
private:
    std::unordered_map<std::type_index, native_type> types_;
};

}

#endif
