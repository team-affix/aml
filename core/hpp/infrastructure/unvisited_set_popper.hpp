#ifndef UNVISITED_SET_POPPER_HPP
#define UNVISITED_SET_POPPER_HPP

#include <optional>
#include "value_objects/queued_set.hpp"

namespace AML {

// Pops the next set not visited yet at its cost, discarding the visited ones it pops on the way and recording the one
// it returns as visited. There is none once the source is empty.
template<typename IPopSet, typename IMeasureSetKey, typename ITryVisit>
struct unvisited_set_popper {
    unvisited_set_popper(IPopSet& pop_set, IMeasureSetKey& measure_set_key, ITryVisit& try_visit);
    std::optional<queued_set> pop();
private:
    IPopSet& pop_set_;
    IMeasureSetKey& measure_set_key_;
    ITryVisit& try_visit_;
};

#define UNVISITED_SET_POPPER_TEMPLATE template<typename IPopSet, typename IMeasureSetKey, typename ITryVisit>
#define UNVISITED_SET_POPPER          unvisited_set_popper<IPopSet, IMeasureSetKey, ITryVisit>

UNVISITED_SET_POPPER_TEMPLATE
UNVISITED_SET_POPPER::unvisited_set_popper(IPopSet& pop_set, IMeasureSetKey& measure_set_key, ITryVisit& try_visit)
    : pop_set_(pop_set)
    , measure_set_key_(measure_set_key)
    , try_visit_(try_visit) {}

UNVISITED_SET_POPPER_TEMPLATE
std::optional<queued_set> UNVISITED_SET_POPPER::pop() {
    while (true) {
        const std::optional<queued_set> popped = pop_set_.pop();
        if (!popped.has_value())
            return popped;
        if (try_visit_.try_visit(measure_set_key_.measure(popped.value().members), popped.value().cost))
            return popped;
    }
}

#undef UNVISITED_SET_POPPER
#undef UNVISITED_SET_POPPER_TEMPLATE

}

#endif
