#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include <functional>
#include <vector>
#include "value_objects/value.hpp"

namespace AML {

// A native primitive: a total function from argument values to a result value. It must make its result through the
// same value pool the solver uses, so results are interned.
using primitive = std::function<const value*(const std::vector<const value*>&)>;

}

#endif
