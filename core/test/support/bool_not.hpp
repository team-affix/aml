#ifndef BOOL_NOT_HPP
#define BOOL_NOT_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Boolean negation over a user boolean type with the constructors `false_ctor` and `true_ctor`.
struct bool_not {
    bool_not(AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
