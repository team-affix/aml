#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include "value_objects/lc_expr.hpp"

// Drains globals LIFO (last-defined first).  That places the last-defined term
// innermost (de Bruijn 0) and the first-defined outermost, matching scope order.
// The innermost body is always nullptr (program skeleton).
template<typename IMakeLcAbs, typename IMakeLcApp, typename IGlobalsStack>
struct assembler {
    assembler(IMakeLcAbs& make_abs, IMakeLcApp& make_app,
              IGlobalsStack& globals);

    const lc_expr* assemble();

private:
    IMakeLcAbs&     make_abs_;
    IMakeLcApp&     make_app_;
    IGlobalsStack&  globals_;
};

template<typename IL, typename IA, typename IG>
assembler<IL, IA, IG>::assembler(IL& make_abs, IA& make_app, IG& globals)
    : make_abs_(make_abs)
    , make_app_(make_app)
    , globals_(globals) {}

template<typename IL, typename IA, typename IG>
const lc_expr* assembler<IL, IA, IG>::assemble() {
    const lc_expr* result = nullptr;
    while (!globals_.empty())
        result = make_app_.make_app(make_abs_.make_abs(result), globals_.pop());
    return result;
}

#endif
