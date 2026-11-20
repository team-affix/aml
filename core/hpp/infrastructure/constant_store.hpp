#ifndef CONSTANT_STORE_HPP
#define CONSTANT_STORE_HPP

#include <cstddef>
#include <vector>
#include "value_objects/constant_operand.hpp"
#include "value_objects/type_id.hpp"

namespace AML {

// Stores the constants synthesis may use as primitive arguments, in insertion order and by type.
struct constant_store {
    constant_store();
    void insert(const constant_operand& constant);
    const std::vector<constant_operand>& constants() const;
    const std::vector<size_t>& constants_of_type(type_id type) const;
private:
    std::vector<constant_operand> constants_;
    std::vector<std::vector<size_t>> by_type_;
    std::vector<size_t> none_;
};

}

#endif
