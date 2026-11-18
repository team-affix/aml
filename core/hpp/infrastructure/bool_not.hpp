#ifndef BOOL_NOT_HPP
#define BOOL_NOT_HPP

#include <vector>
#include "value_objects/boolean_encoding.hpp"
#include "value_objects/primitive_signature.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean negation.
template<typename IMakeValue>
struct bool_not {
    bool_not(IMakeValue& make_value, const boolean_encoding& booleans);
    primitive_signature signature() const;
    const value* apply(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    boolean_encoding booleans_;
};

#define BOOL_NOT_TEMPLATE template<typename IMakeValue>
#define BOOL_NOT          bool_not<IMakeValue>

BOOL_NOT_TEMPLATE
BOOL_NOT::bool_not(IMakeValue& make_value, const boolean_encoding& booleans)
    : make_value_(make_value)
    , booleans_(booleans) {}

BOOL_NOT_TEMPLATE
primitive_signature BOOL_NOT::signature() const {
    return primitive_signature{.arg_types = {booleans_.type}, .result_type = booleans_.type};
}

BOOL_NOT_TEMPLATE
const value* BOOL_NOT::apply(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool operand = args[0]->ctor == booleans_.true_ctor;
    return make_value_.make(operand ? booleans_.false_ctor : booleans_.true_ctor, {});
}

#undef BOOL_NOT
#undef BOOL_NOT_TEMPLATE

#endif
