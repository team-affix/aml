#ifndef OPERAND_DATA_HPP
#define OPERAND_DATA_HPP

#include <any>

namespace AML {

// An operand's values as a primitive reads them: a column (a std::vector of its type in an any), or, when scalar, one
// value (in an any) that every row reads.
struct operand_data {
    const std::any* data;
    bool scalar;
    auto operator<=>(const operand_data&) const = default;
};

}

#endif
