#include "infrastructure/set_key_store.hpp"

namespace AML {

set_key_store::set_key_store()
    : smallest_() {}

bool set_key_store::try_record(uint64_t key, size_t ancestry_size) {
    const auto [found, inserted] = smallest_.try_emplace(key, ancestry_size);
    if (inserted)
        return true;
    if (found->second <= ancestry_size)
        return false;
    found->second = ancestry_size;
    return true;
}

}
