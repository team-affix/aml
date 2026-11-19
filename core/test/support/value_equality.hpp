#ifndef VALUE_EQUALITY_HPP
#define VALUE_EQUALITY_HPP

#include <vector>
#include "value_objects/ctor_ref.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Equality of two values of one type, answered in a user boolean type with the constructors `false_ctor` and
// `true_ctor`. Values are interned, so equal values share a pointer.
template<typename IMakeValue>
struct value_equality {
    value_equality(IMakeValue& make_value, ctor_ref false_ctor, ctor_ref true_ctor);
    const value* operator()(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    ctor_ref false_ctor_;
    ctor_ref true_ctor_;
};

#define VALUE_EQUALITY_TEMPLATE template<typename IMakeValue>
#define VALUE_EQUALITY          value_equality<IMakeValue>

VALUE_EQUALITY_TEMPLATE
VALUE_EQUALITY::value_equality(IMakeValue& make_value, ctor_ref false_ctor, ctor_ref true_ctor)
    : make_value_(make_value)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

VALUE_EQUALITY_TEMPLATE
const value* VALUE_EQUALITY::operator()(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool equal = args[0] == args[1];
    return make_value_.make(equal ? true_ctor_ : false_ctor_, {});
}

#undef VALUE_EQUALITY
#undef VALUE_EQUALITY_TEMPLATE

#endif
