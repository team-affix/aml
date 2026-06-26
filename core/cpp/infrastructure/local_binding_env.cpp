#include "infrastructure/local_binding_env.hpp"

void local_binding_env::push(std::string name) {
    slot_[std::move(name)] = depth_;
    ++depth_;
}

uint32_t local_binding_env::depth() const {
    return depth_;
}

uint32_t local_binding_env::lookup_local(const std::string& name) const {
    return depth_ - 1 - slot_.at(name);
}
