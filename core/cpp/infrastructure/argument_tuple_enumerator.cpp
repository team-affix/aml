#include "infrastructure/argument_tuple_enumerator.hpp"
#include "debug_assert.hpp"

std::vector<std::vector<size_t>> argument_tuple_enumerator::enumerate(
    const std::vector<type_id>& arg_types,
    const std::vector<candidate>& operands,
    size_t frontier_begin,
    size_t frontier_end) const {
    DEBUG_ASSERT(!arg_types.empty());
    std::vector<std::vector<size_t>> tuples;

    std::vector<std::vector<size_t>> matches(arg_types.size());
    for (size_t position = 0; position < arg_types.size(); ++position) {
        for (size_t operand = 0; operand < operands.size(); ++operand) {
            if (operands[operand].observed.type != arg_types[position])
                continue;
            matches[position].push_back(operand);
        }
        if (matches[position].empty())
            return tuples;
    }

    std::vector<size_t> digits(arg_types.size(), 0);
    while (true) {
        std::vector<size_t> tuple;
        bool touches_frontier = false;
        for (size_t position = 0; position < digits.size(); ++position) {
            const size_t operand = matches[position][digits[position]];
            const bool in_frontier = frontier_begin <= operand && operand < frontier_end;
            touches_frontier = touches_frontier || in_frontier;
            tuple.push_back(operand);
        }
        if (touches_frontier)
            tuples.push_back(tuple);

        bool advanced = false;
        for (size_t position = digits.size(); position > 0 && !advanced; --position) {
            size_t& digit = digits[position - 1];
            ++digit;
            advanced = digit < matches[position - 1].size();
            if (!advanced)
                digit = 0;
        }
        if (!advanced)
            return tuples;
    }
}
