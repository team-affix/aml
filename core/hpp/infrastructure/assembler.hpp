#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include "value_objects/lc_expr.hpp"

template<typename IMakeLcAbs, typename IMakeLcApp, typename IPopLcExpr>
struct assembler {
    // globals must be a stack built in definition order: push first-defined first,
    // last-defined last (top of stack).  Popping LIFO naturally places the
    // last-defined term innermost (de Bruijn 0) and the first-defined outermost,
    // which matches the order in which names were pushed to scope during transpilation.
    assembler(IMakeLcAbs& make_abs, IMakeLcApp& make_app, IPopLcExpr& globals);

    // Drains globals and returns the closed chain of let-bindings around nullptr:
    //   app(abs(…app(abs(nullptr), top)…), bottom)
    const lc_expr* assemble();

private:
    IMakeLcAbs& make_abs_;
    IMakeLcApp& make_app_;
    IPopLcExpr& globals_;
};

template<typename IL, typename IA, typename IP>
assembler<IL, IA, IP>::assembler(IL& make_abs, IA& make_app, IP& globals)
    : make_abs_(make_abs), make_app_(make_app), globals_(globals) {}

template<typename IL, typename IA, typename IP>
const lc_expr* assembler<IL, IA, IP>::assemble() {
    const lc_expr* result = nullptr;
    while (!globals_.empty()) {
        result = make_app_.make_app(make_abs_.make_abs(result), globals_.top());
        globals_.pop();
    }
    return result;
}

#endif
