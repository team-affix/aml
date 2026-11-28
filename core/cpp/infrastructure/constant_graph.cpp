#include "infrastructure/constant_graph.hpp"

namespace AML {

constant_graph::constant_graph()
    : records_()
    , by_type_()
    , none_() {}

constant_id constant_graph::insert(const constant_record& record) {
    records_.push_back(record);
    by_type_[record.type].push_back(records_.size() - 1);
    return records_.size() - 1;
}

const constant_record& constant_graph::record(constant_id constant) const {
    return records_.at(constant);
}

const std::vector<constant_id>& constant_graph::of_type(std::type_index type) const {
    const auto found = by_type_.find(type);
    if (found == by_type_.end())
        return none_;
    return found->second;
}

}
