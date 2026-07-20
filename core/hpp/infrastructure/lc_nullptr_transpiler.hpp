#ifndef LC_NULLPTR_TRANSPILER_HPP
#define LC_NULLPTR_TRANSPILER_HPP

#include "value_objects/chc_var_ids.hpp"
#include "value_objects/expr.hpp"

template<typename IMakeVar>
struct lc_nullptr_transpiler {
    lc_nullptr_transpiler(IMakeVar& make_var);

    const expr* transpile_nullptr();

private:
    IMakeVar& make_var_;
};

template<typename IV>
lc_nullptr_transpiler<IV>::lc_nullptr_transpiler(IV& make_var)
    : make_var_(make_var) {}

template<typename IV>
const expr* lc_nullptr_transpiler<IV>::transpile_nullptr() {
    return make_var_.make_var(k_main_function_var_id);
}

#endif
