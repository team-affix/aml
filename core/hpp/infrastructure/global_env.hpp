#ifndef GLOBAL_ENV_HPP
#define GLOBAL_ENV_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct global_env {
    explicit global_env(std::vector<std::string> ordered_names)
        : names_(std::move(ordered_names)) {}

    std::optional<uint32_t> lookup_global(const std::string& name) const {
        for (size_t i = 0; i < names_.size(); ++i) {
            if (names_[i] == name)
                return static_cast<uint32_t>(names_.size() - 1 - i);
        }
        return std::nullopt;
    }

private:
    std::vector<std::string> names_;
};

#endif
