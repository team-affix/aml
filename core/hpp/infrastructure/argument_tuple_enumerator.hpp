#ifndef ARGUMENT_TUPLE_ENUMERATOR_HPP
#define ARGUMENT_TUPLE_ENUMERATOR_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/type_id.hpp"

// Enumerates the tuples of operand indices that type-check against a primitive's argument types and use at least
// one operand from the frontier range [frontier_begin, frontier_end). Tuples come in lexicographic order.
struct argument_tuple_enumerator {
    std::vector<std::vector<size_t>> enumerate(
        const std::vector<type_id>& arg_types,
        const std::vector<candidate>& operands,
        size_t frontier_begin,
        size_t frontier_end) const;
};

#endif
