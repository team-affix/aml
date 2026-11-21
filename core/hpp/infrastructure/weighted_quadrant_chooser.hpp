#ifndef WEIGHTED_QUADRANT_CHOOSER_HPP
#define WEIGHTED_QUADRANT_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/quadrant_key.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses an option by its quadrant key (see quadrant_key.hpp) with weighted odds, for random completions: fresh rather
// than consumed with probability `fresh_weight`, then made rather than given with probability `made_weight`, each
// decided by a drawn fraction; then one of that quadrant's options. A side with no options is never taken, so a
// question with only one possible answer draws nothing, and a quadrant of one option makes no choice within it. With
// both weights 0.5, every quadrant holding options is equally likely when all four do. Returns the chosen option's
// position in `keys`.
template<typename IDrawFraction, typename IChooseInQuadrant>
struct weighted_quadrant_chooser {
    weighted_quadrant_chooser(
        IDrawFraction& draw_fraction, IChooseInQuadrant& choose_in_quadrant, double fresh_weight, double made_weight);
    size_t choose(const std::vector<size_t>& keys);
private:
    IDrawFraction& draw_fraction_;
    IChooseInQuadrant& choose_in_quadrant_;
    double fresh_weight_;
    double made_weight_;
};

#define WEIGHTED_QUADRANT_CHOOSER_TEMPLATE template<typename IDrawFraction, typename IChooseInQuadrant>
#define WEIGHTED_QUADRANT_CHOOSER          weighted_quadrant_chooser<IDrawFraction, IChooseInQuadrant>

WEIGHTED_QUADRANT_CHOOSER_TEMPLATE
WEIGHTED_QUADRANT_CHOOSER::weighted_quadrant_chooser(
    IDrawFraction& draw_fraction, IChooseInQuadrant& choose_in_quadrant, double fresh_weight, double made_weight)
    : draw_fraction_(draw_fraction)
    , choose_in_quadrant_(choose_in_quadrant)
    , fresh_weight_(fresh_weight)
    , made_weight_(made_weight) {}

WEIGHTED_QUADRANT_CHOOSER_TEMPLATE
size_t WEIGHTED_QUADRANT_CHOOSER::choose(const std::vector<size_t>& keys) {
    DEBUG_ASSERT(!keys.empty());
    size_t counts[4] = {0, 0, 0, 0};
    for (const size_t key : keys)
        ++counts[key];
    const bool any_fresh = counts[k_fresh_quadrant_bit] + counts[k_fresh_quadrant_bit + k_made_quadrant_bit] > 0;
    const bool any_consumed = counts[0] + counts[k_made_quadrant_bit] > 0;
    const bool fresh = any_fresh && any_consumed ? draw_fraction_.draw() < fresh_weight_ : any_fresh;
    const size_t freshness = fresh ? k_fresh_quadrant_bit : 0;
    const bool any_made = counts[freshness + k_made_quadrant_bit] > 0;
    const bool any_given = counts[freshness] > 0;
    const bool made = any_made && any_given ? draw_fraction_.draw() < made_weight_ : any_made;
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
