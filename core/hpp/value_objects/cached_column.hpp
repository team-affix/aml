#ifndef CACHED_COLUMN_HPP
#define CACHED_COLUMN_HPP

#include <any>
#include <cstddef>
#include <typeindex>
#include <vector>

namespace AML {

// A feature's values over every training row: their type, the cells as a std::vector of that type in an any, each
// row's class (rows holding equal values share a class, numbered from 0 in order of first appearance), and the
// column's hash. Not comparable, since an any is not.
struct cached_column {
    std::type_index type;
    std::any cells;
    std::vector<size_t> classes;
    size_t hash;
};

}

#endif
