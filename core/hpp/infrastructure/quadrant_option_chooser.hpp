#ifndef QUADRANT_OPTION_CHOOSER_HPP
#define QUADRANT_OPTION_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/quadrant_key.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses an option by its quadrant key (see quadrant_key.hpp) in two flat choices: one among the quadrants that hold
// options, made and fresh first, then given and fresh, made and consumed, and given and consumed; then one among that
// quadrant's options, in their given order. One quadrant makes no quadrant choice, and a quadrant of one option makes
// no choice within it. Returns the chosen option's position in `keys`.
template<typename IChooseQuadrant, typename IChooseInQuadrant>
struct quadrant_option_chooser {
    quadrant_option_chooser(IChooseQuadrant& choose_quadrant, IChooseInQuadrant& choose_in_quadrant);
    size_t choose(const std::vector<size_t>& keys);
private:
    IChooseQuadrant& choose_quadrant_;
    IChooseInQuadrant& choose_in_quadrant_;
};

#define QUADRANT_OPTION_CHOOSER_TEMPLATE template<typename IChooseQuadrant, typename IChooseInQuadrant>
#define QUADRANT_OPTION_CHOOSER          quadrant_option_chooser<IChooseQuadrant, IChooseInQuadrant>

QUADRANT_OPTION_CHOOSER_TEMPLATE
QUADRANT_OPTION_CHOOSER::quadrant_option_chooser(
    IChooseQuadrant& choose_quadrant, IChooseInQuadrant& choose_in_quadrant)
    : choose_quadrant_(choose_quadrant)
    , choose_in_quadrant_(choose_in_quadrant) {}

QUADRANT_OPTION_CHOOSER_TEMPLATE
size_t QUADRANT_OPTION_CHOOSER::choose(const std::vector<size_t>& keys) {
    DEBUG_ASSERT(!keys.empty());
    size_t counts[4] = {0, 0, 0, 0};
    for (const size_t key : keys)
        ++counts[key];
    std::vector<size_t> quadrants;
    for (size_t key = 4; key-- > 0;) {
        if (counts[key] == 0)
            continue;
        quadrants.push_back(key);
    }
    const bool one_quadrant = quadrants.size() == 1;
    const size_t quadrant = quadrants.at(one_quadrant ? 0 : choose_quadrant_.choose(quadrants.size()));
    std::vector<size_t> members;
    for (size_t position = 0; position < keys.size(); ++position) {
        if (keys[position] != quadrant)
            continue;
        members.push_back(position);
    }
    if (members.size() == 1)
        return members.front();
    return members.at(choose_in_quadrant_.choose(members.size()));
}

#undef QUADRANT_OPTION_CHOOSER
#undef QUADRANT_OPTION_CHOOSER_TEMPLATE

}

#endif
