#ifndef QUADRANT_KEY_HPP
#define QUADRANT_KEY_HPP

#include <cstddef>

namespace AML {

// An operand's quadrant as a key: whether it is fresh (nothing in the session has taken it as an argument yet) and
// whether it is made (synthesized) rather than given (an input, a bound argument or a constant). Made and fresh is 3,
// given and fresh 2, made and consumed 1, given and consumed 0.
const size_t k_fresh_quadrant_bit = 2;
const size_t k_made_quadrant_bit = 1;

}

#endif
