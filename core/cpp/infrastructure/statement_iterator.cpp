#include "infrastructure/statement_iterator.hpp"

statement_iterator::statement_iterator(const std::vector<statement_file>& files)
    : files_(files)
    , file_idx_(0)
    , item_idx_(0) {}

std::optional<statement> statement_iterator::get_next_statement() {
    while (file_idx_ < files_.size() &&
           item_idx_ >= files_.at(file_idx_).statements.size()) {
        ++file_idx_;
        item_idx_ = 0;
    }
    if (file_idx_ >= files_.size())
        return std::nullopt;
    return files_.at(file_idx_).statements.at(item_idx_++);
}
