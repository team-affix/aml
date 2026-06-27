#ifndef APP_TRANSPILER_HPP
#define APP_TRANSPILER_HPP

#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"

template<typename ITranspileExpr, typename IMakeLcApp>
struct app_transpiler {
    app_transpiler(ITranspileExpr& transpile_expr, IMakeLcApp& make_app);

    const lc_expr* transpile_app(const aml_expr::app& a);

private:
    ITranspileExpr& transpile_expr_;
    IMakeLcApp&     make_app_;
};

template<typename IT, typename IA>
app_transpiler<IT, IA>::app_transpiler(IT& transpile_expr, IA& make_app)
    : transpile_expr_(transpile_expr), make_app_(make_app) {}

template<typename IT, typename IA>
const lc_expr* app_transpiler<IT, IA>::transpile_app(const aml_expr::app& a) {
    return make_app_.make_app(transpile_expr_.transpile(a.func),
                              transpile_expr_.transpile(a.arg));
}

#endif
