#include "infrastructure/uniform_column_detector.hpp"

#include <algorithm>

namespace AML {

bool uniform_column_detector::detect(const cached_column& column) const {
    if (column.classes.empty())
        return false;
    return *std::max_element(column.classes.begin(), column.classes.end()) == 0;
}

}
