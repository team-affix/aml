#include "infrastructure/feature_graph.hpp"

namespace AML {

feature_graph::feature_graph()
    : records_() {}

feature_id feature_graph::insert(const feature_record& record) {
    records_.push_back(record);
    return records_.size() - 1;
}

void feature_graph::replace_origin(feature_id feature, const derivation& origin) {
    records_.at(feature).origin = origin;
}

const feature_record& feature_graph::record(feature_id feature) const {
    return records_.at(feature);
}

}
