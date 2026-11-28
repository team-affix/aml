#ifndef DOUBLE_VECTOR_LENGTH_HPP
#define DOUBLE_VECTOR_LENGTH_HPP

#include <vector>

// The number of elements in a vector of doubles.
struct double_vector_length {
    int operator()(const std::vector<double>& elements) const;
};

#endif
