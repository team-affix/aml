#ifndef COLUMN_HPP
#define COLUMN_HPP

#include <vector>
#include "value_objects/value.hpp"

namespace AML {

// One interned value per row of a bin, aligned with the bin's rows.
using column = std::vector<const value*>;

}

#endif
