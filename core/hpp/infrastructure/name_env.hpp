#ifndef NAME_ENV_HPP
#define NAME_ENV_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct local_binding_env {
    std::vector<std::string> stack;

    void push(std::string name) { stack.push_back(std::move(name)); }
    uint32_t depth() const { return static_cast<uint32_t>(stack.size()); }

    std::optional<uint32_t> lookup_local(const std::string& name) const {
        for (size_t i = stack.size(); i > 0; --i) {
            if (stack[i - 1] == name)
                return static_cast<uint32_t>(stack.size() - i);
        }
        return std::nullopt;
    }
};

struct global_env {
    std::vector<std::string> names;

    global_env() = default;

    explicit global_env(std::vector<std::string> ordered_names)
        : names(std::move(ordered_names)) {}

    std::optional<uint32_t> lookup_global(const std::string& name) const {
        for (size_t i = 0; i < names.size(); ++i) {
            if (names[i] == name)
                return static_cast<uint32_t>(names.size() - 1 - i);
        }
        return std::nullopt;
    }
};

#endif
