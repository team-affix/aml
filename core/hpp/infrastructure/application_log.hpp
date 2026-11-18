#ifndef APPLICATION_LOG_HPP
#define APPLICATION_LOG_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_application.hpp"
#include "value_objects/prim_id.hpp"

// Stores the primitive applications a synthesis session has tried.
struct application_log {
    application_log();
    void clear();
    void record(const prim_application& application);
    size_t count() const;
    size_t count_with_prefix(prim_id prim, const std::vector<size_t>& prefix) const;
private:
    std::vector<prim_application> applications_;
};

#endif
