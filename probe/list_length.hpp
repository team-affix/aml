#ifndef LIST_LENGTH_HPP
#define LIST_LENGTH_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// A list's number of elements, over a user list type with the constructors `nil_ctor` and `cons_ctor(head, tail)`,
// as a Peano natural with the constructors `zero_ctor` and `suc_ctor(predecessor)`.
struct list_length {
    list_length(AML::ctor_id nil_ctor, AML::ctor_id zero_ctor, AML::ctor_id suc_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id nil_ctor_;
    AML::ctor_id zero_ctor_;
    AML::ctor_id suc_ctor_;
};

#endif
