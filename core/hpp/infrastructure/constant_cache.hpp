#ifndef CONSTANT_CACHE_HPP
#define CONSTANT_CACHE_HPP

#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>
#include "value_objects/cached_constant.hpp"
#include "value_objects/constant_id.hpp"

namespace AML {

// Stores constants' cached values within a byte budget. Finding a value makes it the most recently used; inserting one
// evicts the least recently used others until the held bytes fit the budget again, so a single value larger than the
// budget is still held. Values are shared, so a reader keeps an evicted value alive while it holds it.
struct constant_cache {
    constant_cache(size_t byte_budget);
    void insert(constant_id constant, std::shared_ptr<const cached_constant> cached, size_t bytes);
    std::shared_ptr<const cached_constant> find(constant_id constant);
    size_t held_bytes() const;
private:
    struct slot {
        std::shared_ptr<const cached_constant> cached;
        size_t bytes;
        std::list<constant_id>::iterator recency;
    };
    size_t byte_budget_;
    size_t held_bytes_;
    std::list<constant_id> recency_;
    std::unordered_map<constant_id, slot> slots_;
};

}

#endif
