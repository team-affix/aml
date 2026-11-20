#include "infrastructure/output_finder.hpp"

namespace AML {

std::optional<feature> output_finder::find(
    const std::vector<candidate>& candidates, const typed_column& labels) const {
    const candidate* cheapest = nullptr;
    for (const candidate& entry : candidates) {
        if (entry.observed != labels)
            continue;
        if (cheapest != nullptr && entry.cost >= cheapest->cost)
            continue;
        cheapest = &entry;
    }
    if (cheapest == nullptr)
        return std::nullopt;
    return cheapest->expression;
}

}
