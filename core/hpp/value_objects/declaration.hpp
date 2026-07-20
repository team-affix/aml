#ifndef DECLARATION_HPP
#define DECLARATION_HPP

#include <cstdint>
#include <string>

struct declaration {
    std::string name;
    uint32_t    arity;
    auto operator<=>(const declaration&) const = default;
};

#endif
