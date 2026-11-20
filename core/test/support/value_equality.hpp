#ifndef VALUE_EQUALITY_HPP
#define VALUE_EQUALITY_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Equality of two values of one type, compared by structure, answered in a user boolean type with the constructors
// `false_ctor` and `true_ctor`.
struct value_equality {
    value_equality(AML::ctor_id false_ctor, AML::ctor_id true_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id false_ctor_;
    AML::ctor_id true_ctor_;
};

#endif
