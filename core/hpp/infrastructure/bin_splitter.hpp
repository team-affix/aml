#ifndef BIN_SPLITTER_HPP
#define BIN_SPLITTER_HPP

#include <cstddef>
#include <utility>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/ctor_def.hpp"
#include "value_objects/type_def.hpp"
#include "value_objects/typed_column.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

namespace AML {

// Splits a bin by a one-layer case on a scrutinee column: one child per constructor of the scrutinee's type, in id
// order. A child holds the rows whose scrutinee has its constructor, with that constructor's arguments bound as new
// variables after the parent's inputs. A constructor no row has gives an empty child.
template<typename IGetTypeDef>
struct bin_splitter {
    bin_splitter(IGetTypeDef& get_type_def);
    std::vector<bin> split(const bin& parent, const typed_column& scrutinee) const;
private:
    IGetTypeDef& get_type_def_;
};

#define BIN_SPLITTER_TEMPLATE template<typename IGetTypeDef>
#define BIN_SPLITTER          bin_splitter<IGetTypeDef>

BIN_SPLITTER_TEMPLATE
BIN_SPLITTER::bin_splitter(IGetTypeDef& get_type_def)
    : get_type_def_(get_type_def) {}

BIN_SPLITTER_TEMPLATE
std::vector<bin> BIN_SPLITTER::split(const bin& parent, const typed_column& scrutinee) const {
    DEBUG_ASSERT(scrutinee.cells.size() == parent.labels.cells.size());
    const std::vector<ctor_def>& ctors = get_type_def_.def(scrutinee.type).ctors;
    std::vector<size_t> row_counts(ctors.size(), 0);
    for (const value& matched : scrutinee.cells)
        ++row_counts.at(matched.ctor);

    std::vector<bin> children;
    children.reserve(ctors.size());
    for (size_t ctor = 0; ctor < ctors.size(); ++ctor) {
        bin child{.inputs = {}, .labels = typed_column{.type = parent.labels.type, .cells = {}}};
        child.inputs.reserve(parent.inputs.size() + ctors[ctor].arg_types.size());
        for (const typed_column& variable : parent.inputs)
            child.inputs.push_back(typed_column{.type = variable.type, .cells = {}});
        for (const type_id arg_type : ctors[ctor].arg_types)
            child.inputs.push_back(typed_column{.type = arg_type, .cells = {}});
        for (typed_column& input : child.inputs)
            input.cells.reserve(row_counts[ctor]);
        child.labels.cells.reserve(row_counts[ctor]);
        children.push_back(std::move(child));
    }

    for (size_t row = 0; row < parent.labels.cells.size(); ++row) {
        const value& matched = scrutinee.cells[row];
        bin& child = children.at(matched.ctor);
        for (size_t index = 0; index < parent.inputs.size(); ++index)
            child.inputs[index].cells.push_back(parent.inputs[index].cells[row]);
        for (size_t argument = 0; argument < matched.args.size(); ++argument)
            child.inputs[parent.inputs.size() + argument].cells.push_back(matched.args[argument]);
        child.labels.cells.push_back(parent.labels.cells[row]);
    }
    return children;
}

#undef BIN_SPLITTER
#undef BIN_SPLITTER_TEMPLATE

}

#endif
