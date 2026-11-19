#ifndef FIRST_OPTION_CHOOSER_HPP
#define FIRST_OPTION_CHOOSER_HPP

#include <cstddef>

// Chooses the first option.
struct first_option_chooser {
    size_t choose(size_t option_count) const;
};

#endif
