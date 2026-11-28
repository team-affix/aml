#include "infrastructure/visited_set_store.hpp"

namespace AML {

visited_set_store::visited_set_store()
    : visited_() {}

bool visited_set_store::try_visit(uint64_t key, size_t cost) {
    return visited_.emplace(key, cost).second;
}

}
