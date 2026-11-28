#ifndef CONSTANT_GRAPH_HPP
#define CONSTANT_GRAPH_HPP

#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/constant_id.hpp"
#include "value_objects/constant_record.hpp"

namespace AML {

// Stores every constant, each with its origin and type, by id and by type in insertion order. A constant is
// never removed.
struct constant_graph {
    constant_graph();
    constant_id insert(const constant_record& record);
    const constant_record& record(constant_id constant) const;
    const std::vector<constant_id>& of_type(std::type_index type) const;
private:
    std::vector<constant_record> records_;
    std::unordered_map<std::type_index, std::vector<constant_id>> by_type_;
    std::vector<constant_id> none_;
};

}

#endif
