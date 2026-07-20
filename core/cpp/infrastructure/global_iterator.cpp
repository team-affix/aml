#include "infrastructure/global_iterator.hpp"

global_iterator::global_iterator(const std::vector<module_file>& files)
    : files_(files)
    , file_idx_(0)
    , item_idx_(0) {}

std::optional<global> global_iterator::get_next_global() {
    while (file_idx_ < files_.size() &&
           item_idx_ >= files_.at(file_idx_).items.size()) {
        ++file_idx_;
        item_idx_ = 0;
    }
    if (file_idx_ >= files_.size())
        return std::nullopt;
    return files_.at(file_idx_).items.at(item_idx_++);
}
