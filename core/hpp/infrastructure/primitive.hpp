#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include <variant>
#include "infrastructure/bool_and.hpp"
#include "infrastructure/bool_not.hpp"
#include "infrastructure/bool_or.hpp"
#include "infrastructure/nat_add.hpp"
#include "infrastructure/value_equality.hpp"
#include "infrastructure/value_pool.hpp"

// The native primitives a solver can be given. Each is owned by the caller and referred to here.
using primitive = std::variant<
    const bool_not<value_pool>*,
    const bool_and<value_pool>*,
    const bool_or<value_pool>*,
    const value_equality<value_pool>*,
    const nat_add<value_pool>*>;

#endif
