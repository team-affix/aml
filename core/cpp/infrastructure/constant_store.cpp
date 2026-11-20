#include "infrastructure/constant_store.hpp"

namespace AML {

constant_store::constant_store()
    : constants_()
    , by_type_()
    , none_() {}

void constant_store::insert(const constant_operand& constant) {
    if (constant.type >= by_type_.size())
        by_type_.resize(constant.type + 1);
    by_type_[constant.type].push_back(constants_.size());
    constants_.push_back(constant);
}

const std::vector<constant_operand>& constant_store::constants() const {
    return constants_;
}

const std::vector<size_t>& constant_store::constants_of_type(type_id type) const {
    if (type >= by_type_.size())
        return none_;
    return by_type_[type];
}

}
