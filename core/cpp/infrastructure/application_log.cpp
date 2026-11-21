#include "infrastructure/application_log.hpp"

#include <algorithm>

namespace AML {

application_log::application_log()
    : applications_() {}

void application_log::clear() {
    applications_.clear();
}

void application_log::record(const prim_application& application) {
    applications_.push_back(application);
}

size_t application_log::count() const {
    return applications_.size();
}

size_t application_log::count_with_prefix(prim_id prim, const std::vector<size_t>& prefix) const {
    size_t matching = 0;
    for (const prim_application& application : applications_) {
        if (application.prim != prim)
            continue;
        const bool starts_with = std::equal(prefix.begin(), prefix.end(), application.args.begin());
        if (!starts_with)
            continue;
        ++matching;
    }
    return matching;
}

bool application_log::is_used(size_t operand) const {
    for (const prim_application& application : applications_) {
        if (std::find(application.args.begin(), application.args.end(), operand) != application.args.end())
            return true;
    }
    return false;
}

}
