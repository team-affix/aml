#ifndef UNIFORM_COLUMN_DETECTOR_HPP
#define UNIFORM_COLUMN_DETECTOR_HPP

#include "value_objects/cached_column.hpp"

namespace AML {

// Detects a column holding one value in every row: every row is in class 0. A column with no rows is not uniform.
struct uniform_column_detector {
    bool detect(const cached_column& column) const;
};

}

#endif
