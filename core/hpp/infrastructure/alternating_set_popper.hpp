#ifndef ALTERNATING_SET_POPPER_HPP
#define ALTERNATING_SET_POPPER_HPP

#include <optional>
#include "value_objects/queued_set.hpp"

namespace AML {

// Pops from two sources in turn, the first source first. A source with nothing to give passes its turn to the other;
// there is none only when both are empty.
template<typename IPopSet0, typename IPopSet1>
struct alternating_set_popper {
    alternating_set_popper(IPopSet0& pop_set0, IPopSet1& pop_set1);
    std::optional<queued_set> pop();
private:
    IPopSet0& pop_set0_;
    IPopSet1& pop_set1_;
    bool second_turn_;
};

#define ALTERNATING_SET_POPPER_TEMPLATE template<typename IPopSet0, typename IPopSet1>
#define ALTERNATING_SET_POPPER          alternating_set_popper<IPopSet0, IPopSet1>

ALTERNATING_SET_POPPER_TEMPLATE
ALTERNATING_SET_POPPER::alternating_set_popper(IPopSet0& pop_set0, IPopSet1& pop_set1)
    : pop_set0_(pop_set0)
    , pop_set1_(pop_set1)
    , second_turn_(false) {}

ALTERNATING_SET_POPPER_TEMPLATE
std::optional<queued_set> ALTERNATING_SET_POPPER::pop() {
    const bool second = second_turn_;
    second_turn_ = !second_turn_;
    const std::optional<queued_set> popped = second ? pop_set1_.pop() : pop_set0_.pop();
    if (popped.has_value())
        return popped;
    return second ? pop_set0_.pop() : pop_set1_.pop();
}

#undef ALTERNATING_SET_POPPER
#undef ALTERNATING_SET_POPPER_TEMPLATE

}

#endif
