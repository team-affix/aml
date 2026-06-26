#ifndef BUILTIN_CONSTRUCTORS_HPP
#define BUILTIN_CONSTRUCTORS_HPP

#include <array>
#include <string_view>
#include "value_objects/constructor_group.hpp"

inline constexpr std::array<std::string_view, 6> builtin_constructor_names = {
    "true", "false", "cons", "nil", "pos", "negsuc",
};

inline const constructor_group builtin_bool_group{{{"true", 0}, {"false", 0}}};
inline const constructor_group builtin_list_group{{{"cons", 2}, {"nil", 0}}};
inline const constructor_group builtin_int_group{{{"pos", 1}, {"negsuc", 1}}};

#endif
