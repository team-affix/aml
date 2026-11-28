#ifndef SET_STRATUM_HPP
#define SET_STRATUM_HPP

#include <cstddef>

namespace AML {

// The stratum a queued set falls in, the "type" of Xie et al.'s type-based exploration: its ancestry size, the cost of
// what it has built, and its distinct rows, the cost of what it still leaves to the table. Sets alike in both share
// a bucket.
struct set_stratum {
    size_t ancestry_size;
    size_t distinct_rows;
    auto operator<=>(const set_stratum&) const = default;
};

}

#endif
