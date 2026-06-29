#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include "value_objects/lc_expr.hpp"

template<typename IMakeLcAbs, typename IMakeLcApp,
         typename ITopLcExpr, typename IPopLcExpr>
struct assembler {
    // globals_top  — provides top(): peek at the next term to bind.
    // globals_pop  — provides empty() and pop(): controls loop termination and advance.
    // Both references typically point at the same stack object; they are split so
    // each concern can be mocked independently in tests.
    //
    // The stack must be built in definition order: push first-defined first,
    // last-defined last.  LIFO popping places the last-defined term innermost
    // (de Bruijn 0) and the first-defined outermost, matching the scope push order.
    assembler(IMakeLcAbs& make_abs, IMakeLcApp& make_app,
              ITopLcExpr& globals_top, IPopLcExpr& globals_pop);

    // Drains globals and returns the closed chain of let-bindings around nullptr:
    //   app(abs(…app(abs(nullptr), top)…), bottom)
    const lc_expr* assemble();

private:
    IMakeLcAbs& make_abs_;
    IMakeLcApp& make_app_;
    ITopLcExpr& top_;
    IPopLcExpr& pop_;
};

template<typename IL, typename IA, typename IT, typename IP>
assembler<IL, IA, IT, IP>::assembler(IL& make_abs, IA& make_app,
                                     IT& globals_top, IP& globals_pop)
    : make_abs_(make_abs), make_app_(make_app),
      top_(globals_top), pop_(globals_pop) {}

template<typename IL, typename IA, typename IT, typename IP>
const lc_expr* assembler<IL, IA, IT, IP>::assemble() {
    const lc_expr* result = nullptr;
    while (!pop_.empty()) {
        result = make_app_.make_app(make_abs_.make_abs(result), top_.top());
        pop_.pop();
    }
    return result;
}

#endif
