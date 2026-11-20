#ifndef LIST_PRIMITIVES_HPP
#define LIST_PRIMITIVES_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// General-purpose primitives over a user list type with the constructors `nil_ctor` and `cons_ctor(head, tail)`,
// answering in a user boolean type with `false_ctor` and `true_ctor` where they answer a truth value.

// Whether a list is empty.
struct list_is_nil {
    list_is_nil(AML::ctor_id nil_ctor, AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id nil_ctor_;
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

// A list's first element, or `fallback` for the empty list.
struct list_head {
    list_head(AML::ctor_id nil_ctor, AML::value fallback);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id nil_ctor_;
    AML::value fallback_;
};

// A list without its first element; the empty list's tail is the empty list.
struct list_tail {
    list_tail(AML::ctor_id nil_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id nil_ctor_;
};

#endif
