#ifndef NATIVE_CLASSIFIER_HPP
#define NATIVE_CLASSIFIER_HPP

#include <any>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace AML {

// Gives each row of a column of T, held in an any as a std::vector<T>, its class: rows holding equal values share a
// class, and classes are numbered from 0 in order of first appearance, so every class is below the row count.
template<typename T, typename Hash>
struct native_classifier {
    std::vector<size_t> operator()(const std::any& cells) const;
};

#define NATIVE_CLASSIFIER_TEMPLATE template<typename T, typename Hash>
#define NATIVE_CLASSIFIER          native_classifier<T, Hash>

NATIVE_CLASSIFIER_TEMPLATE
std::vector<size_t> NATIVE_CLASSIFIER::operator()(const std::any& cells) const {
    const std::vector<T>& typed = std::any_cast<const std::vector<T>&>(cells);
    std::unordered_map<T, size_t, Hash> classes;
    std::vector<size_t> row_classes;
    row_classes.reserve(typed.size());
    for (const T& cell : typed) {
        const auto inserted = classes.try_emplace(cell, classes.size());
        row_classes.push_back(inserted.first->second);
    }
    return row_classes;
}

#undef NATIVE_CLASSIFIER
#undef NATIVE_CLASSIFIER_TEMPLATE

}

#endif
