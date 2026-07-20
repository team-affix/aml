#ifndef GLOBAL_STACK_HPP
#define GLOBAL_STACK_HPP

#include <vector>
#include "value_objects/lc_expr.hpp"

struct global_stack {
    global_stack();

    void push(const lc_expr* term);
    const lc_expr* pop();
    bool empty() const;

private:
    std::vector<const lc_expr*> terms_;
};

#endif
