#ifndef GLOBAL_ITERATOR_HPP
#define GLOBAL_ITERATOR_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/global.hpp"
#include "value_objects/module_file.hpp"

struct global_iterator {
    global_iterator(const std::vector<module_file>& files);
    std::optional<global> get_next_global();

private:
    const std::vector<module_file>& files_;
    size_t file_idx_;
    size_t item_idx_;
};

#endif
