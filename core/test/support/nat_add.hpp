#ifndef NAT_ADD_HPP
#define NAT_ADD_HPP

#include <cstddef>
#include <vector>
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Addition over a user Peano type whose successor constructor is at tag `suc_tag`: wraps the left operand in one
// successor per successor of the right operand.
template<typename IMakeValue>
struct nat_add {
    nat_add(IMakeValue& make_value, size_t suc_tag);
    const AML::value* operator()(const std::vector<const AML::value*>& args) const;
private:
    IMakeValue& make_value_;
    size_t suc_tag_;
};

#define NAT_ADD_TEMPLATE template<typename IMakeValue>
#define NAT_ADD          nat_add<IMakeValue>

NAT_ADD_TEMPLATE
NAT_ADD::nat_add(IMakeValue& make_value, size_t suc_tag)
    : make_value_(make_value)
    , suc_tag_(suc_tag) {}

NAT_ADD_TEMPLATE
const AML::value* NAT_ADD::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const AML::value* sum = args[0];
    for (const AML::value* rest = args[1]; rest->tag == suc_tag_; rest = rest->args[0])
        sum = make_value_.make(suc_tag_, {sum});
    return sum;
}

#undef NAT_ADD
#undef NAT_ADD_TEMPLATE

#endif
