#include "infrastructure/output_finder.hpp"

std::optional<const feature*> output_finder::find(
    const std::vector<candidate>& candidates, const column& labels) const {
    const candidate* cheapest = nullptr;
    for (const candidate& entry : candidates) {
        if (entry.observed.cells != labels)
            continue;
        if (cheapest != nullptr && entry.cost >= cheapest->cost)
            continue;
        cheapest = &entry;
    }
    if (cheapest == nullptr)
        return std::nullopt;
    return cheapest->expression;
}
