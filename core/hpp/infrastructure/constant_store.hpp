#ifndef CONSTANT_STORE_HPP
#define CONSTANT_STORE_HPP

#include <vector>
#include "value_objects/constant_operand.hpp"

// Stores the constants synthesis may use as primitive arguments.
struct constant_store {
    constant_store();
    void insert(const constant_operand& constant);
    const std::vector<constant_operand>& constants() const;
private:
    std::vector<constant_operand> constants_;
};

#endif
