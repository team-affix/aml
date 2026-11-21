#ifndef LIST_HEAD_HPP
#define LIST_HEAD_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// A list's first element, or `fallback` for the empty list, over a user list type with the constructors `nil_ctor` and
// `cons_ctor(head, tail)`.
struct list_head {
    list_head(AML::ctor_id nil_ctor, AML::value fallback);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id nil_ctor_;
    AML::value fallback_;
};

#endif
