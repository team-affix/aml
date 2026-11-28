#ifndef SET_QUEUE_HPP
#define SET_QUEUE_HPP

#include <cstddef>
#include <map>
#include <optional>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/queued_set.hpp"

namespace AML {

// Stores the sets waiting to be visited, cheapest first, and sets of equal cost in the order they came. Past its
// capacity the costliest set, the latest among equals, is dropped.
struct set_queue {
    set_queue(size_t capacity);
    void push(const queued_set& entry);
    std::optional<queued_set> pop();
    size_t size() const;
private:
    size_t capacity_;
    std::multimap<size_t, std::vector<feature_id>> sets_;
};

}

#endif
