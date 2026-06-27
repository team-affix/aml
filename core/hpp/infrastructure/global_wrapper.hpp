#ifndef GLOBAL_WRAPPER_HPP
#define GLOBAL_WRAPPER_HPP

#include <vector>
#include "value_objects/lc_expr.hpp"

template<typename IMakeLcAbs, typename IMakeLcApp>
struct global_wrapper {
    global_wrapper(IMakeLcAbs& make_abs, IMakeLcApp& make_app);

    // globals[i] was pre-pushed at index i (outermost first).
    // Iterates in reverse so the last-pushed global wraps innermost.
    const lc_expr* wrap(const lc_expr* main, const std::vector<const lc_expr*>& globals);

private:
    IMakeLcAbs& make_abs_;
    IMakeLcApp& make_app_;
};

template<typename IL, typename IA>
global_wrapper<IL, IA>::global_wrapper(IL& make_abs, IA& make_app)
    : make_abs_(make_abs), make_app_(make_app) {}

template<typename IL, typename IA>
const lc_expr* global_wrapper<IL, IA>::wrap(const lc_expr* main,
                                            const std::vector<const lc_expr*>& globals) {
    const lc_expr* result = main;
    for (auto it = globals.rbegin(); it != globals.rend(); ++it)
        result = make_app_.make_app(make_abs_.make_abs(result), *it);
    return result;
}

#endif
