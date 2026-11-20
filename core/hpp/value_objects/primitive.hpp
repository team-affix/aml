#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include <functional>
#include <vector>
#include "value_objects/value.hpp"

namespace AML {

// A native primitive: a total function from argument values to a result value. The arguments are read in place (they
// point into the solver's columns); the result is a fresh value the primitive builds.
using primitive = std::function<value(const std::vector<const value*>&)>;

}

#endif
