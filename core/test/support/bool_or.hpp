#ifndef BOOL_OR_HPP
#define BOOL_OR_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Boolean disjunction over a user boolean type with the constructors `false_ctor` and `true_ctor`.
struct bool_or {
    bool_or(AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
