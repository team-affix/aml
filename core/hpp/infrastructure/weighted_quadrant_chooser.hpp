#ifndef WEIGHTED_QUADRANT_CHOOSER_HPP
#define WEIGHTED_QUADRANT_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/quadrant_key.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses an option by its quadrant key (see quadrant_key.hpp) in two questions, for random completions: fresh or
// consumed, as a four-way choice of which three answers mean fresh, so fresh is taken three times in four; then made or
// given, as a two-way choice (made first); then one option of that quadrant. A question with one possible answer is not
// asked, nor is a quadrant of one option. Returns the chosen option's position in `keys`.
template<typename IChooseFreshness, typename IChooseOrigin, typename IChooseInQuadrant>
struct weighted_quadrant_chooser {
    weighted_quadrant_chooser(
        IChooseFreshness& choose_freshness, IChooseOrigin& choose_origin, IChooseInQuadrant& choose_in_quadrant);
    size_t choose(const std::vector<size_t>& keys);
private:
    IChooseFreshness& choose_freshness_;
    IChooseOrigin& choose_origin_;
    IChooseInQuadrant& choose_in_quadrant_;
};

#define WEIGHTED_QUADRANT_CHOOSER_TEMPLATE \
    template<typename IChooseFreshness, typename IChooseOrigin, typename IChooseInQuadrant>
#define WEIGHTED_QUADRANT_CHOOSER \
    weighted_quadrant_chooser<IChooseFreshness, IChooseOrigin, IChooseInQuadrant>

WEIGHTED_QUADRANT_CHOOSER_TEMPLATE
WEIGHTED_QUADRANT_CHOOSER::weighted_quadrant_chooser(
    IChooseFreshness& choose_freshness, IChooseOrigin& choose_origin, IChooseInQuadrant& choose_in_quadrant)
    : choose_freshness_(choose_freshness)
    , choose_origin_(choose_origin)
    , choose_in_quadrant_(choose_in_quadrant) {}

WEIGHTED_QUADRANT_CHOOSER_TEMPLATE
size_t WEIGHTED_QUADRANT_CHOOSER::choose(const std::vector<size_t>& keys) {
    DEBUG_ASSERT(!keys.empty());
    size_t counts[4] = {0, 0, 0, 0};
    for (const size_t key : keys)
        ++counts[key];
    const bool any_fresh = counts[k_fresh_quadrant_bit] + counts[k_fresh_quadrant_bit + k_made_quadrant_bit] > 0;
    const bool any_consumed = counts[0] + counts[k_made_quadrant_bit] > 0;
    const bool ask_freshness = any_fresh && any_consumed;
    const bool fresh = ask_freshness ? choose_freshness_.choose(4) < 3 : any_fresh;
    const size_t freshness = fresh ? k_fresh_quadrant_bit : 0;
    const bool any_made = counts[freshness + k_made_quadrant_bit] > 0;
    const bool any_given = counts[freshness] > 0;
    const bool ask_origin = any_made && any_given;
    const bool made = ask_origin ? choose_origin_.choose(2) == 0 : any_made;
    const size_t quadrant = freshness + (made ? k_made_quadrant_bit : 0);
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

#undef WEIGHTED_QUADRANT_CHOOSER
#undef WEIGHTED_QUADRANT_CHOOSER_TEMPLATE

}

#endif
