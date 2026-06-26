#ifndef TRAINING_FILE_HPP
#define TRAINING_FILE_HPP

#include <vector>
#include "aml_expr.hpp"

struct training_file {
    std::vector<const aml_expr*> statements;
};

#endif
