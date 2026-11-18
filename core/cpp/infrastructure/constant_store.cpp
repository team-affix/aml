#include "infrastructure/constant_store.hpp"

constant_store::constant_store()
    : constants_() {}

void constant_store::insert(const constant_operand& constant) {
    constants_.push_back(constant);
}

const std::vector<constant_operand>& constant_store::constants() const {
    return constants_;
}
