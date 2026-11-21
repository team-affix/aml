#ifndef LIST_IS_NIL_HPP
#define LIST_IS_NIL_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Whether a list is empty, over a user list type with the constructors `nil_ctor` and `cons_ctor(head, tail)`,
// answering in a user boolean type with `false_ctor` and `true_ctor`.
struct list_is_nil {
    list_is_nil(AML::ctor_id nil_ctor, AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id nil_ctor_;
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
