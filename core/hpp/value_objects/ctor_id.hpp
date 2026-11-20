#ifndef CTOR_ID_HPP
#define CTOR_ID_HPP

#include <cstdint>

namespace AML {

// A constructor's id: its position among its type's constructors, as the schema hands it out. Unique only within its
// type.
using ctor_id = uint32_t;

}

#endif
