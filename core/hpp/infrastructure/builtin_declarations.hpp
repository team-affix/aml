#ifndef BUILTIN_DECLARATIONS_HPP
#define BUILTIN_DECLARATIONS_HPP

#include <array>
#include "value_objects/declaration_group.hpp"

inline const declaration_group builtin_bool_group{{{"true", 0}, {"false", 0}}};
inline const declaration_group builtin_list_group{{{"cons", 2}, {"nil", 0}}};
inline const declaration_group builtin_int_group{{{"pos", 1}, {"negsuc", 1}}};

inline const std::array<const declaration_group*, 3> builtin_declaration_groups = {
    &builtin_bool_group, &builtin_list_group, &builtin_int_group,
};

#endif
