#ifndef DEBUG_ASSERT_HPP
#define DEBUG_ASSERT_HPP

#ifdef DEBUG
#include <stdexcept>
#define DEBUG_ASSERT(cond) if (!(cond)) throw std::logic_error(#cond)
#else
#define DEBUG_ASSERT(cond) ((void)0)
#endif

#endif
