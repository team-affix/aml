#include "infrastructure/training_data.hpp"

training_data::training_data()
    : next_idx_(0) {}

void training_data::push_data_point(data_point point) {
    points_.push_back(point);
}

std::optional<data_point> training_data::get_next_data_point() {
    if (next_idx_ >= points_.size())
        return std::nullopt;
    return points_.at(next_idx_++);
}
