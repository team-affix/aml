#ifndef FEATURE_GRAPH_HPP
#define FEATURE_GRAPH_HPP

#include <vector>
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"

namespace AML {

// Stores every feature ever made, by id. A feature is never removed; only its origin may be replaced.
struct feature_graph {
    feature_graph();
    feature_id insert(const feature_record& record);
    void replace_origin(feature_id feature, const derivation& origin);
    const feature_record& record(feature_id feature) const;
private:
    std::vector<feature_record> records_;
};

}

#endif
