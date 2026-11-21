#ifndef QUADRANT_OPTION_CHOOSER_HPP
#define QUADRANT_OPTION_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/quadrant_key.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses an option by its quadrant key (see quadrant_key.hpp) in three choices: made (option 0) or given (option 1);
// then fresh (option 0) or consumed (option 1); then one of that quadrant's options, in their given order. A two-way
// choice with options on only one side is not asked, nor is a choice within a quadrant of one option. Returns the
// chosen option's position in `keys`.
template<typename IChooseOrigin, typename IChooseFreshness, typename IChooseInQuadrant>
struct quadrant_option_chooser {
    quadrant_option_chooser(
        IChooseOrigin& choose_origin, IChooseFreshness& choose_freshness, IChooseInQuadrant& choose_in_quadrant);
    size_t choose(const std::vector<size_t>& keys);
private:
    IChooseOrigin& choose_origin_;
    IChooseFreshness& choose_freshness_;
    IChooseInQuadrant& choose_in_quadrant_;
};

#define QUADRANT_OPTION_CHOOSER_TEMPLATE \
    template<typename IChooseOrigin, typename IChooseFreshness, typename IChooseInQuadrant>
#define QUADRANT_OPTION_CHOOSER          quadrant_option_chooser<IChooseOrigin, IChooseFreshness, IChooseInQuadrant>

QUADRANT_OPTION_CHOOSER_TEMPLATE
QUADRANT_OPTION_CHOOSER::quadrant_option_chooser(
    IChooseOrigin& choose_origin, IChooseFreshness& choose_freshness, IChooseInQuadrant& choose_in_quadrant)
    : choose_origin_(choose_origin)
    , choose_freshness_(choose_freshness)
    , choose_in_quadrant_(choose_in_quadrant) {}

QUADRANT_OPTION_CHOOSER_TEMPLATE
size_t QUADRANT_OPTION_CHOOSER::choose(const std::vector<size_t>& keys) {
    DEBUG_ASSERT(!keys.empty());
    size_t counts[4] = {0, 0, 0, 0};
    for (const size_t key : keys)
        ++counts[key];
    const bool any_made = counts[k_made_quadrant_bit] + counts[k_fresh_quadrant_bit + k_made_quadrant_bit] > 0;
    const bool any_given = counts[0] + counts[k_fresh_quadrant_bit] > 0;
    const bool made = any_made && any_given ? choose_origin_.choose(2) == 0 : any_made;
    const size_t origin = made ? k_made_quadrant_bit : 0;
    const bool any_fresh = counts[origin + k_fresh_quadrant_bit] > 0;
    const bool any_consumed = counts[origin] > 0;
    const bool fresh = any_fresh && any_consumed ? choose_freshness_.choose(2) == 0 : any_fresh;
    const size_t quadrant = origin + (fresh ? k_fresh_quadrant_bit : 0);
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
