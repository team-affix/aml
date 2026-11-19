#ifndef NAT_ADD_HPP
#define NAT_ADD_HPP

#include <vector>
#include "value_objects/nat_encoding.hpp"
#include "value_objects/primitive_signature.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Addition of Peano naturals: wraps the left operand in one successor per successor of the right operand.
template<typename IMakeValue>
struct nat_add {
    nat_add(IMakeValue& make_value, const nat_encoding& nats);
    primitive_signature signature() const;
    const value* apply(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    nat_encoding nats_;
};

#define NAT_ADD_TEMPLATE template<typename IMakeValue>
#define NAT_ADD          nat_add<IMakeValue>

NAT_ADD_TEMPLATE
NAT_ADD::nat_add(IMakeValue& make_value, const nat_encoding& nats)
    : make_value_(make_value)
    , nats_(nats) {}

NAT_ADD_TEMPLATE
primitive_signature NAT_ADD::signature() const {
    return primitive_signature{.arg_types = {nats_.type, nats_.type}, .result_type = nats_.type};
}

NAT_ADD_TEMPLATE
const value* NAT_ADD::apply(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const value* sum = args[0];
    for (const value* rest = args[1]; rest->ctor == nats_.suc_ctor; rest = rest->args[0])
        sum = make_value_.make(nats_.suc_ctor, {sum});
    return sum;
}

#undef NAT_ADD
#undef NAT_ADD_TEMPLATE

#endif
