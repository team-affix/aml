#ifndef LIST_TAIL_HPP
#define LIST_TAIL_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// A list without its first element, over a user list type with the constructors `nil_ctor` and
// `cons_ctor(head, tail)`; the empty list's tail is the empty list.
struct list_tail {
    list_tail(AML::ctor_id nil_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id nil_ctor_;
};

#endif
