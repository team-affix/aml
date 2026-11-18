#ifndef FIRST_CANDIDATE_CHOOSER_HPP
#define FIRST_CANDIDATE_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"

// Chooses the first candidate.
struct first_candidate_chooser {
    size_t choose(const std::vector<candidate>& candidates) const;
};

#endif
