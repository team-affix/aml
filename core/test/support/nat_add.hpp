#ifndef NAT_ADD_HPP
#define NAT_ADD_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// Addition over a user Peano type whose successor constructor is `suc_ctor`: wraps the left operand in one successor
// per successor of the right operand.
struct nat_add {
    nat_add(AML::ctor_id suc_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id suc_ctor_;
};

#endif
