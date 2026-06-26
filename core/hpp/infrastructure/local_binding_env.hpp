#ifndef LOCAL_BINDING_ENV_HPP
#define LOCAL_BINDING_ENV_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

struct local_binding_env {
    void push(std::string name);
    uint32_t depth() const;

    uint32_t lookup_local(const std::string& name) const;

private:
    uint32_t depth_ = 0;
    std::unordered_map<std::string, uint32_t> slot_;
};

#endif
