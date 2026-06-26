#ifndef CONSTRUCTOR_DECL_HPP
#define CONSTRUCTOR_DECL_HPP

#include <cstdint>
#include <string>

struct constructor_decl {
    std::string name;
    uint32_t    arity;
    auto operator<=>(const constructor_decl&) const = default;
};

#endif
