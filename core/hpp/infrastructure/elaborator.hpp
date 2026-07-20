#ifndef ELABORATOR_HPP
#define ELABORATOR_HPP

#include <vector>
#include "value_objects/chc_var_ids.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/lc_functor_ids.hpp"

template<typename IPumpGlobals,
         typename IPumpStatements,
         typename IAssemble,
         typename ITranspileLcExpr,
         typename IGetNextDataPoint,
         typename ITranspileDataPoint,
         typename IMakeVar,
         typename IMakeFunctor,
         typename IPushInitialGoal>
struct elaborator {
    elaborator(IPumpGlobals& pump_globals,
               IPumpStatements& pump_statements,
               IAssemble& assemble,
               ITranspileLcExpr& transpile_lc_expr,
               IGetNextDataPoint& get_next_data_point,
               ITranspileDataPoint& transpile_data_point,
               IMakeVar& make_var,
               IMakeFunctor& make_functor,
               IPushInitialGoal& push_initial_goal);

    void elaborate();

private:
    IPumpGlobals&         pump_globals_;
    IPumpStatements&      pump_statements_;
    IAssemble&            assemble_;
    ITranspileLcExpr&     transpile_lc_expr_;
    IGetNextDataPoint&    get_next_data_point_;
    ITranspileDataPoint&  transpile_data_point_;
    IMakeVar&             make_var_;
    IMakeFunctor&         make_functor_;
    IPushInitialGoal&     push_initial_goal_;
};

template<typename IG, typename IS, typename IA, typename IT, typename ID,
         typename IDP, typename IV, typename IF, typename IP>
elaborator<IG, IS, IA, IT, ID, IDP, IV, IF, IP>::elaborator(
        IG& pump_globals,
        IS& pump_statements,
        IA& assemble,
        IT& transpile_lc_expr,
        ID& get_next_data_point,
        IDP& transpile_data_point,
        IV& make_var,
        IF& make_functor,
        IP& push_initial_goal)
    : pump_globals_(pump_globals)
    , pump_statements_(pump_statements)
    , assemble_(assemble)
    , transpile_lc_expr_(transpile_lc_expr)
    , get_next_data_point_(get_next_data_point)
    , transpile_data_point_(transpile_data_point)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , push_initial_goal_(push_initial_goal) {}

template<typename IG, typename IS, typename IA, typename IT, typename ID,
         typename IDP, typename IV, typename IF, typename IP>
void elaborator<IG, IS, IA, IT, ID, IDP, IV, IF, IP>::elaborate() {
    pump_globals_.pump();
    pump_statements_.pump();
    const lc_expr* program = assemble_.assemble();
    const expr* P = transpile_lc_expr_.transpile(program);
    const expr* M = make_var_.make_var(k_model_var_id);
    push_initial_goal_.push(
        make_functor_.make_functor(k_eq_functor_id, {M, P}));
    while (auto dp = get_next_data_point_.get_next_data_point())
        push_initial_goal_.push(
            transpile_data_point_.transpile_data_point(*dp));
}

#endif
