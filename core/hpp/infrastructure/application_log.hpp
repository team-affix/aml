#ifndef APPLICATION_LOG_HPP
#define APPLICATION_LOG_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_application.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Stores the primitive applications a synthesis session has tried. An operand is used once any of them takes it as an
// argument.
struct application_log {
    application_log();
    void clear();
    void record(const prim_application& application);
    size_t count() const;
    size_t count_with_prefix(prim_id prim, const std::vector<size_t>& prefix) const;
    bool is_used(size_t operand) const;
private:
    std::vector<prim_application> applications_;
};

}

#endif
