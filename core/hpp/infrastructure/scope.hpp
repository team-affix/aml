#ifndef SCOPE_HPP
#define SCOPE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct scope {
    void     push(std::string name);
    void     pop();
    uint32_t get_var_index(const std::string& name) const;

private:
    uint32_t depth_ = 0;
    std::unordered_map<std::string, std::vector<uint32_t>> slot_;    // name → stack of intro_depths
    std::unordered_map<uint32_t, std::string>              reverse_; // intro_depth → name
};

#endif
