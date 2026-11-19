#include "infrastructure/feature_order_measurer.hpp"

#include <algorithm>
#include <variant>

size_t feature_order_measurer::measure(const feature& expression) const {
    const feature::app* application = std::get_if<feature::app>(&expression.content);
    if (application == nullptr)
        return 0;
    size_t highest = 0;
    for (const feature& arg : application->args)
        highest = std::max(highest, measure(arg));
    return highest + 1;
}
