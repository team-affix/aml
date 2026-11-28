#include "infrastructure/row_key_numberer.hpp"

#include <unordered_map>

namespace AML {

std::vector<std::vector<size_t>> row_key_numberer::number(
    const std::vector<const std::vector<size_t>*>& columns, size_t row_count) const {
    std::vector<std::vector<size_t>> layers;
    layers.reserve(columns.size() + 1);
    layers.emplace_back(row_count, 0);
    for (const std::vector<size_t>* classes : columns) {
        const std::vector<size_t>& previous = layers.back();
        std::unordered_map<size_t, size_t> numbers;
        std::vector<size_t> layer;
        layer.reserve(row_count);
        for (size_t row = 0; row < row_count; ++row) {
            const size_t pair = previous[row] * row_count + (*classes)[row];
            const auto inserted = numbers.try_emplace(pair, numbers.size());
            layer.push_back(inserted.first->second);
        }
        layers.push_back(std::move(layer));
    }
    return layers;
}

}
