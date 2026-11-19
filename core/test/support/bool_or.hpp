#ifndef BOOL_OR_HPP
#define BOOL_OR_HPP

#include <vector>
#include "value_objects/ctor_ref.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean disjunction over a user boolean type with the constructors `false_ctor` and `true_ctor`.
template<typename IMakeValue>
struct bool_or {
    bool_or(IMakeValue& make_value, ctor_ref false_ctor, ctor_ref true_ctor);
    const value* operator()(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    ctor_ref false_ctor_;
    ctor_ref true_ctor_;
};

#define BOOL_OR_TEMPLATE template<typename IMakeValue>
#define BOOL_OR          bool_or<IMakeValue>

BOOL_OR_TEMPLATE
BOOL_OR::bool_or(IMakeValue& make_value, ctor_ref false_ctor, ctor_ref true_ctor)
    : make_value_(make_value)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

BOOL_OR_TEMPLATE
const value* BOOL_OR::operator()(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool left = args[0]->ctor == true_ctor_;
    const bool right = args[1]->ctor == true_ctor_;
    const bool result = left || right;
    return make_value_.make(result ? true_ctor_ : false_ctor_, {});
}

#undef BOOL_OR
#undef BOOL_OR_TEMPLATE

#endif
