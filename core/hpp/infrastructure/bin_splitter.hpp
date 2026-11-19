#ifndef BIN_SPLITTER_HPP
#define BIN_SPLITTER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/ctor_def.hpp"
#include "value_objects/type_def.hpp"
#include "value_objects/typed_column.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Splits a bin by a one-layer case on a scrutinee column: one child per constructor of the scrutinee's type, in tag
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
    DEBUG_ASSERT(scrutinee.cells.size() == parent.labels.size());
    std::vector<bin> children;
    for (const ctor_def& ctor : get_type_def_.def(scrutinee.type).ctors) {
        bin child;
        for (const typed_column& variable : parent.inputs)
            child.inputs.push_back(typed_column{.type = variable.type, .cells = {}});
        for (const type_id arg_type : ctor.arg_types)
            child.inputs.push_back(typed_column{.type = arg_type, .cells = {}});
        children.push_back(child);
    }

    for (size_t row = 0; row < parent.labels.size(); ++row) {
        const value* matched = scrutinee.cells[row];
        DEBUG_ASSERT(matched->ctor.type == scrutinee.type);
        bin& child = children.at(matched->ctor.tag);
        for (size_t index = 0; index < parent.inputs.size(); ++index)
            child.inputs[index].cells.push_back(parent.inputs[index].cells[row]);
        for (size_t argument = 0; argument < matched->args.size(); ++argument)
            child.inputs[parent.inputs.size() + argument].cells.push_back(matched->args[argument]);
        child.labels.push_back(parent.labels[row]);
    }
    return children;
}

#undef BIN_SPLITTER
#undef BIN_SPLITTER_TEMPLATE

#endif
