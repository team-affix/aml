#include "infrastructure/scope.hpp"

void scope::push(std::string name) {
    ++depth_;
    slot_[name].push_back(depth_);
    reverse_[depth_] = name;
}

void scope::pop() {
    std::string name = reverse_.at(depth_);
    reverse_.erase(depth_);
    auto& stack = slot_.at(name);
    stack.pop_back();
    if (stack.empty())
        slot_.erase(name);
    --depth_;
}

bool scope::contains(const std::string& name) const {
    return slot_.find(name) != slot_.end();
}

uint32_t scope::get_var_index(const std::string& name) const {
    return depth_ - slot_.at(name).back();
}
