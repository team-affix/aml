#include "support/double_vector_length.hpp"

int double_vector_length::operator()(const std::vector<double>& elements) const {
    return static_cast<int>(elements.size());
}
