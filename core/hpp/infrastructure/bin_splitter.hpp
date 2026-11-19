#ifndef BIN_SPLITTER_HPP
#define BIN_SPLITTER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/ctor_decl.hpp"
#include "value_objects/ctor_id.hpp"
#include "value_objects/typed_column.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Splits a bin by a one-layer case on a scrutinee column: one child per constructor of the scrutinee's type, in tag
// order. A child holds the rows whose scrutinee has its constructor, with that constructor's arguments bound as new
// variables after the parent's scope. A constructor no row has gives an empty child.
template<typename IGetTypeCtors, typename IGetCtorDecl>
struct bin_splitter {
    bin_splitter(IGetTypeCtors& get_type_ctors, IGetCtorDecl& get_ctor_decl);
    std::vector<bin> split(const bin& parent, const typed_column& scrutinee) const;
private:
    IGetTypeCtors& get_type_ctors_;
    IGetCtorDecl& get_ctor_decl_;
};

#define BIN_SPLITTER_TEMPLATE template<typename IGetTypeCtors, typename IGetCtorDecl>
#define BIN_SPLITTER          bin_splitter<IGetTypeCtors, IGetCtorDecl>

BIN_SPLITTER_TEMPLATE
BIN_SPLITTER::bin_splitter(IGetTypeCtors& get_type_ctors, IGetCtorDecl& get_ctor_decl)
    : get_type_ctors_(get_type_ctors)
    , get_ctor_decl_(get_ctor_decl) {}

BIN_SPLITTER_TEMPLATE
std::vector<bin> BIN_SPLITTER::split(const bin& parent, const typed_column& scrutinee) const {
    DEBUG_ASSERT(scrutinee.cells.size() == parent.labels.size());
    std::vector<bin> children;
    for (const ctor_id ctor : get_type_ctors_.ctors(scrutinee.type)) {
        bin child;
        for (const typed_column& variable : parent.scope)
            child.scope.push_back(typed_column{.type = variable.type, .cells = {}});
        for (const type_id arg_type : get_ctor_decl_.decl(ctor).arg_types)
            child.scope.push_back(typed_column{.type = arg_type, .cells = {}});
        children.push_back(child);
    }

    for (size_t row = 0; row < parent.labels.size(); ++row) {
        const value* matched = scrutinee.cells[row];
        bin& child = children[get_ctor_decl_.decl(matched->ctor).tag];
        for (size_t index = 0; index < parent.scope.size(); ++index)
            child.scope[index].cells.push_back(parent.scope[index].cells[row]);
        for (size_t argument = 0; argument < matched->args.size(); ++argument)
            child.scope[parent.scope.size() + argument].cells.push_back(matched->args[argument]);
        child.labels.push_back(parent.labels[row]);
    }
    return children;
}

#undef BIN_SPLITTER
#undef BIN_SPLITTER_TEMPLATE

#endif
