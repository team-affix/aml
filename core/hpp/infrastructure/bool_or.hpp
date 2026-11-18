#ifndef BOOL_OR_HPP
#define BOOL_OR_HPP

#include <vector>
#include "value_objects/boolean_encoding.hpp"
#include "value_objects/primitive_signature.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean disjunction.
template<typename IMakeValue>
struct bool_or {
    bool_or(IMakeValue& make_value, const boolean_encoding& booleans);
    primitive_signature signature() const;
    const value* apply(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    boolean_encoding booleans_;
};

#define BOOL_OR_TEMPLATE template<typename IMakeValue>
#define BOOL_OR          bool_or<IMakeValue>

BOOL_OR_TEMPLATE
BOOL_OR::bool_or(IMakeValue& make_value, const boolean_encoding& booleans)
    : make_value_(make_value)
    , booleans_(booleans) {}

BOOL_OR_TEMPLATE
primitive_signature BOOL_OR::signature() const {
    return primitive_signature{.arg_types = {booleans_.type, booleans_.type}, .result_type = booleans_.type};
}

BOOL_OR_TEMPLATE
const value* BOOL_OR::apply(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool left = args[0]->ctor == booleans_.true_ctor;
    const bool right = args[1]->ctor == booleans_.true_ctor;
    const bool result = left || right;
    return make_value_.make(result ? booleans_.true_ctor : booleans_.false_ctor, {});
}

#undef BOOL_OR
#undef BOOL_OR_TEMPLATE

#endif
