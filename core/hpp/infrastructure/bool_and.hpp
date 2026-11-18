#ifndef BOOL_AND_HPP
#define BOOL_AND_HPP

#include <vector>
#include "value_objects/boolean_encoding.hpp"
#include "value_objects/primitive_signature.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean conjunction.
template<typename IMakeValue>
struct bool_and {
    bool_and(IMakeValue& make_value, const boolean_encoding& booleans);
    primitive_signature signature() const;
    const value* apply(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    boolean_encoding booleans_;
};

#define BOOL_AND_TEMPLATE template<typename IMakeValue>
#define BOOL_AND          bool_and<IMakeValue>

BOOL_AND_TEMPLATE
BOOL_AND::bool_and(IMakeValue& make_value, const boolean_encoding& booleans)
    : make_value_(make_value)
    , booleans_(booleans) {}

BOOL_AND_TEMPLATE
primitive_signature BOOL_AND::signature() const {
    return primitive_signature{.arg_types = {booleans_.type, booleans_.type}, .result_type = booleans_.type};
}

BOOL_AND_TEMPLATE
const value* BOOL_AND::apply(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool left = args[0]->ctor == booleans_.true_ctor;
    const bool right = args[1]->ctor == booleans_.true_ctor;
    const bool result = left && right;
    return make_value_.make(result ? booleans_.true_ctor : booleans_.false_ctor, {});
}

#undef BOOL_AND
#undef BOOL_AND_TEMPLATE

#endif
