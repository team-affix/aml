#ifndef DECLARATION_DECL_HPP
#define DECLARATION_DECL_HPP

#include <cstdint>
#include <string>

struct declaration_decl {
    std::string name;
    uint32_t    arity;
    auto operator<=>(const declaration_decl&) const = default;
};

#endif
