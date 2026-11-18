#ifndef VALUE_EQUALITY_HPP
#define VALUE_EQUALITY_HPP

#include <vector>
#include "value_objects/boolean_encoding.hpp"
#include "value_objects/primitive_signature.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Equality of two values of one type. Values are interned, so equal values share a pointer.
template<typename IMakeValue>
struct value_equality {
    value_equality(IMakeValue& make_value, type_id operand_type, const boolean_encoding& booleans);
    primitive_signature signature() const;
    const value* apply(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    type_id operand_type_;
    boolean_encoding booleans_;
};

#define VALUE_EQUALITY_TEMPLATE template<typename IMakeValue>
#define VALUE_EQUALITY          value_equality<IMakeValue>

VALUE_EQUALITY_TEMPLATE
VALUE_EQUALITY::value_equality(IMakeValue& make_value, type_id operand_type, const boolean_encoding& booleans)
    : make_value_(make_value)
    , operand_type_(operand_type)
    , booleans_(booleans) {}

VALUE_EQUALITY_TEMPLATE
primitive_signature VALUE_EQUALITY::signature() const {
    return primitive_signature{.arg_types = {operand_type_, operand_type_}, .result_type = booleans_.type};
}

VALUE_EQUALITY_TEMPLATE
const value* VALUE_EQUALITY::apply(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool equal = args[0] == args[1];
    return make_value_.make(equal ? booleans_.true_ctor : booleans_.false_ctor, {});
}

#undef VALUE_EQUALITY
#undef VALUE_EQUALITY_TEMPLATE

#endif
