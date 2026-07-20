#ifndef LC_VAR_TRANSPILER_HPP
#define LC_VAR_TRANSPILER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

template<typename IMakeFunctor>
struct lc_var_transpiler {
    lc_var_transpiler(IMakeFunctor& make_functor);

    const expr* transpile_var(const lc_expr::var& v);

private:
    IMakeFunctor& make_functor_;
};

template<typename IF>
lc_var_transpiler<IF>::lc_var_transpiler(IF& make_functor)
    : make_functor_(make_functor) {}

template<typename IF>
const expr* lc_var_transpiler<IF>::transpile_var(const lc_expr::var& v) {
    const expr* peano = make_functor_.make_functor(k_zero_functor_id, {});
    for (uint32_t i = 0; i < v.index; ++i)
        peano = make_functor_.make_functor(k_suc_functor_id, {peano});
    return make_functor_.make_functor(k_var_functor_id, {peano});
}

#endif
