#ifndef VISITED_SET_STORE_HPP
#define VISITED_SET_STORE_HPP

#include <cstddef>
#include <cstdint>
#include <set>
#include <utility>

namespace AML {

// Stores every set visited, by its key and the cost it was queued at. Visiting tells whether the set was new at that
// cost and records it, so a set queued in two places is visited once, while one queued again at a smaller cost is new.
struct visited_set_store {
    visited_set_store();
    bool try_visit(uint64_t key, size_t cost);
private:
    std::set<std::pair<uint64_t, size_t>> visited_;
};

}

#endif
