#ifndef CHEAPEST_SET_VISITOR_HPP
#define CHEAPEST_SET_VISITOR_HPP

#include <optional>
#include "value_objects/queued_set.hpp"
#include "value_objects/table_model.hpp"
#include "value_objects/table_outcome.hpp"

namespace AML {

// Visits the cheapest queued set: takes it off the queue, records its table model if it is the cheapest yet, and has
// each expander push its children. There is no outcome once the queue is empty.
template<
    typename IPopSet, typename IBuildTableModel, typename IRecordBestTable, typename IExpandSet0,
    typename IExpandSet1>
struct cheapest_set_visitor {
    cheapest_set_visitor(
        IPopSet& pop_set,
        IBuildTableModel& build_table_model,
        IRecordBestTable& record_best_table,
        IExpandSet0& expand_set0,
        IExpandSet1& expand_set1);
    std::optional<table_outcome> visit();
private:
    IPopSet& pop_set_;
    IBuildTableModel& build_table_model_;
    IRecordBestTable& record_best_table_;
    IExpandSet0& expand_set0_;
    IExpandSet1& expand_set1_;
};

#define CHEAPEST_SET_VISITOR_TEMPLATE \
    template< \
        typename IPopSet, typename IBuildTableModel, typename IRecordBestTable, typename IExpandSet0, \
        typename IExpandSet1>
#define CHEAPEST_SET_VISITOR \
    cheapest_set_visitor<IPopSet, IBuildTableModel, IRecordBestTable, IExpandSet0, IExpandSet1>

CHEAPEST_SET_VISITOR_TEMPLATE
CHEAPEST_SET_VISITOR::cheapest_set_visitor(
    IPopSet& pop_set,
    IBuildTableModel& build_table_model,
    IRecordBestTable& record_best_table,
    IExpandSet0& expand_set0,
    IExpandSet1& expand_set1)
    : pop_set_(pop_set)
    , build_table_model_(build_table_model)
    , record_best_table_(record_best_table)
    , expand_set0_(expand_set0)
    , expand_set1_(expand_set1) {}

CHEAPEST_SET_VISITOR_TEMPLATE
std::optional<table_outcome> CHEAPEST_SET_VISITOR::visit() {
    const std::optional<queued_set> cheapest = pop_set_.pop();
    if (!cheapest.has_value())
        return std::nullopt;
    const table_model model = build_table_model_.build(cheapest.value().members);
    record_best_table_.try_record(model, cheapest.value().cost);
    expand_set0_.expand(cheapest.value().members);
    expand_set1_.expand(cheapest.value().members);
    return table_outcome{.model = model, .cost = cheapest.value().cost};
}

#undef CHEAPEST_SET_VISITOR
#undef CHEAPEST_SET_VISITOR_TEMPLATE

}

#endif
