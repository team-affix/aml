#ifndef SET_KEY_STORE_HPP
#define SET_KEY_STORE_HPP

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace AML {

// Stores the key of every set ever queued with the smallest ancestry size it was queued at. Recording a key tells
// whether it was new or its ancestry size smaller than every one recorded for it, and keeps the smaller size.
struct set_key_store {
    set_key_store();
    bool try_record(uint64_t key, size_t ancestry_size);
private:
    std::unordered_map<uint64_t, size_t> smallest_;
};

}

#endif
