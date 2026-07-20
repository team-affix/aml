#ifndef TRAINING_DATA_HPP
#define TRAINING_DATA_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/data_point.hpp"

struct training_data {
    training_data();

    void push_data_point(data_point point);
    std::optional<data_point> get_next_data_point();

private:
    std::vector<data_point> points_;
    size_t next_idx_;
};

#endif
