#ifndef INHERITING_BIN_SPLITTER_HPP
#define INHERITING_BIN_SPLITTER_HPP

#include <cstddef>
#include <memory>
#include <vector>
#include "value_objects/bin_view.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/column_view.hpp"
#include "value_objects/feature.hpp"

namespace AML {

// Splits a bin as the bin splitter it wraps does, then passes the scrutinee's pieces (see scrutinee_piece_finder.hpp)
// down as variables: each piece is evaluated once over the parent's rows, and every child appends it after its bound
// arguments, selecting its own rows, so a child's variables are its parent's, its constructor's arguments, then the
// pieces. Pieces from further up arrive as the parent's variables. The scrutinee's expression is the session's
// candidate that observed the scrutinee column; a column no candidate observed (a funnel's bound argument) is a
// variable, which has no pieces. A piece uniform over a child stays among its variables, so the numbering does not
// depend on the rows; finding a session's variables leaves it out.
template<
    typename ISplitBin,
    typename IGetFeatureCandidates,
    typename IFindScrutineePieces,
    typename IEvaluateFeatureColumn>
struct inheriting_bin_splitter {
    inheriting_bin_splitter(
        ISplitBin& split_bin,
        IGetFeatureCandidates& get_feature_candidates,
        IFindScrutineePieces& find_scrutinee_pieces,
        IEvaluateFeatureColumn& evaluate_feature_column);
    std::vector<bin_view> split(const bin_view& parent, const column_view& scrutinee) const;
private:
    ISplitBin& split_bin_;
    IGetFeatureCandidates& get_feature_candidates_;
    IFindScrutineePieces& find_scrutinee_pieces_;
    IEvaluateFeatureColumn& evaluate_feature_column_;
};

#define INHERITING_BIN_SPLITTER_TEMPLATE template<typename ISB, typename IGFC, typename IFSP, typename IEFC>
#define INHERITING_BIN_SPLITTER          inheriting_bin_splitter<ISB, IGFC, IFSP, IEFC>

INHERITING_BIN_SPLITTER_TEMPLATE
INHERITING_BIN_SPLITTER::inheriting_bin_splitter(
    ISB& split_bin, IGFC& get_feature_candidates, IFSP& find_scrutinee_pieces, IEFC& evaluate_feature_column)
    : split_bin_(split_bin)
    , get_feature_candidates_(get_feature_candidates)
    , find_scrutinee_pieces_(find_scrutinee_pieces)
    , evaluate_feature_column_(evaluate_feature_column) {}

INHERITING_BIN_SPLITTER_TEMPLATE
std::vector<bin_view> INHERITING_BIN_SPLITTER::split(const bin_view& parent, const column_view& scrutinee) const {
    std::vector<bin_view> children = split_bin_.split(parent, scrutinee);
    std::vector<feature> pieces;
    for (const candidate& held : get_feature_candidates_.candidates()) {
        const bool observed_it = held.observed.cells == scrutinee.cells && held.observed.rows == scrutinee.rows;
        if (!observed_it)
            continue;
        pieces = find_scrutinee_pieces_.find(held.expression);
        break;
    }
    if (pieces.empty())
        return children;
    std::vector<column_view> columns;
    columns.reserve(pieces.size());
    for (const feature& piece : pieces)
        columns.push_back(evaluate_feature_column_.evaluate(piece, parent));
    // Each child's rows, as positions among the parent's rows; every piece column is dense over the parent's rows,
    // so one selection serves them all.
    std::vector<std::vector<size_t>> positions(children.size());
    for (size_t row = 0; row < parent.labels.rows->size(); ++row)
        positions.at((*scrutinee.cells)[(*scrutinee.rows)[row]].ctor).push_back(row);
    for (size_t ctor = 0; ctor < children.size(); ++ctor) {
        const auto selected = std::make_shared<const std::vector<size_t>>(std::move(positions[ctor]));
        for (const column_view& column : columns)
            children[ctor].inputs.push_back(column_view{.type = column.type, .cells = column.cells, .rows = selected});
    }
    return children;
}

#undef INHERITING_BIN_SPLITTER
#undef INHERITING_BIN_SPLITTER_TEMPLATE

}

#endif
