#ifndef BOOL_NOT_HPP
#define BOOL_NOT_HPP

#include <vector>
#include "value_objects/ctor_ref.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean negation over a user boolean type with the constructors `false_ctor` and `true_ctor`.
template<typename IMakeValue>
struct bool_not {
    bool_not(IMakeValue& make_value, ctor_ref false_ctor, ctor_ref true_ctor);
    const value* operator()(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    ctor_ref false_ctor_;
    ctor_ref true_ctor_;
};

#define BOOL_NOT_TEMPLATE template<typename IMakeValue>
#define BOOL_NOT          bool_not<IMakeValue>

BOOL_NOT_TEMPLATE
BOOL_NOT::bool_not(IMakeValue& make_value, ctor_ref false_ctor, ctor_ref true_ctor)
    : make_value_(make_value)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

BOOL_NOT_TEMPLATE
const value* BOOL_NOT::operator()(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool operand = args[0]->ctor == true_ctor_;
    return make_value_.make(operand ? false_ctor_ : true_ctor_, {});
}

#undef BOOL_NOT
#undef BOOL_NOT_TEMPLATE

#endif
