#ifndef NAT_ADD_HPP
#define NAT_ADD_HPP

#include <vector>
#include "value_objects/ctor_ref.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Addition over a user Peano type whose successor constructor is `suc_ctor`: wraps the left operand in one successor
// per successor of the right operand.
template<typename IMakeValue>
struct nat_add {
    nat_add(IMakeValue& make_value, ctor_ref suc_ctor);
    const value* operator()(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    ctor_ref suc_ctor_;
};

#define NAT_ADD_TEMPLATE template<typename IMakeValue>
#define NAT_ADD          nat_add<IMakeValue>

NAT_ADD_TEMPLATE
NAT_ADD::nat_add(IMakeValue& make_value, ctor_ref suc_ctor)
    : make_value_(make_value)
    , suc_ctor_(suc_ctor) {}

NAT_ADD_TEMPLATE
const value* NAT_ADD::operator()(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const value* sum = args[0];
    for (const value* rest = args[1]; rest->ctor == suc_ctor_; rest = rest->args[0])
        sum = make_value_.make(suc_ctor_, {sum});
    return sum;
}

#undef NAT_ADD
#undef NAT_ADD_TEMPLATE

#endif
