#ifndef STATEMENT_PROCESSOR_HPP
#define STATEMENT_PROCESSOR_HPP

#include "value_objects/data_point.hpp"
#include "value_objects/statement.hpp"

template<typename ITranspileExpr,
         typename IPushDataPoint>
struct statement_processor {
    statement_processor(ITranspileExpr& transpile_expr,
                        IPushDataPoint& push_data_point);

    void process_statement(const statement& stmt);

private:
    ITranspileExpr&     transpile_expr_;
    IPushDataPoint&     push_data_point_;
};

template<typename ITE, typename IPD>
statement_processor<ITE, IPD>::statement_processor(
        ITE& transpile_expr,
        IPD& push_data_point)
    : transpile_expr_(transpile_expr)
    , push_data_point_(push_data_point) {}

template<typename ITE, typename IPD>
void statement_processor<ITE, IPD>::process_statement(const statement& stmt) {
    push_data_point_.push_data_point({
        transpile_expr_.transpile(stmt.lhs),
        transpile_expr_.transpile(stmt.rhs)});
}

#endif
