#ifndef NAT_MOD_HPP
#define NAT_MOD_HPP

#include <vector>
#include "value_objects/ctor_id.hpp"
#include "value_objects/value.hpp"

// The remainder of one Peano natural divided by another, over a user type with the constructors `zero_ctor` and
// `suc_ctor(predecessor)`. Anything modulo zero is itself.
struct nat_mod {
    nat_mod(AML::ctor_id zero_ctor, AML::ctor_id suc_ctor);
    AML::value operator()(const std::vector<const AML::value*>& args) const;
private:
    AML::ctor_id zero_ctor_;
    AML::ctor_id suc_ctor_;
};

#endif
