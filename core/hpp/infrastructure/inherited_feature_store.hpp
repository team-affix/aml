#ifndef INHERITED_FEATURE_STORE_HPP
#define INHERITED_FEATURE_STORE_HPP

#include <cstddef>
#include <unordered_map>
#include <vector>
#include "value_objects/feature.hpp"

namespace AML {

// Stores, for each bin built below a match, the features its synthesis sessions inherit from the matches above it. A
// bin is known by its labels' selection of rows, which no other live bin shares. A bin without an entry inherits
// nothing.
struct inherited_feature_store {
    inherited_feature_store();
    void clear();
    bool has(const std::vector<size_t>* rows) const;
    void record(const std::vector<size_t>* rows, std::vector<feature> features);
    const std::vector<feature>& inherited(const std::vector<size_t>* rows) const;
private:
    std::unordered_map<const std::vector<size_t>*, std::vector<feature>> by_rows_;
    std::vector<feature> none_;
};

}

#endif
