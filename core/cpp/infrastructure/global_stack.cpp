#include "infrastructure/global_stack.hpp"
#include "debug_assert.hpp"

global_stack::global_stack() {}

void global_stack::push(const lc_expr* term) {
    terms_.push_back(term);
}

const lc_expr* global_stack::pop() {
    DEBUG_ASSERT(!terms_.empty());
    const lc_expr* term = terms_.back();
    terms_.pop_back();
    return term;
}

bool global_stack::empty() const {
    return terms_.empty();
}
