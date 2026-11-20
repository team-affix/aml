#ifndef VALUE_EQUALITY_HPP
#define VALUE_EQUALITY_HPP

#include <cstddef>
#include <vector>
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Equality of two values of one type, answered in a user boolean type with the constructors `false_tag` and
// `true_tag`. Values are interned, so equal values share a pointer.
template<typename IMakeValue>
struct value_equality {
    value_equality(IMakeValue& make_value, size_t false_tag, size_t true_tag);
    const AML::value* operator()(const std::vector<const AML::value*>& args) const;
private:
    IMakeValue& make_value_;
    size_t false_tag_;
    size_t true_tag_;
};

#define VALUE_EQUALITY_TEMPLATE template<typename IMakeValue>
#define VALUE_EQUALITY          value_equality<IMakeValue>

VALUE_EQUALITY_TEMPLATE
VALUE_EQUALITY::value_equality(IMakeValue& make_value, size_t false_tag, size_t true_tag)
    : make_value_(make_value)
    , false_tag_(false_tag)
    , true_tag_(true_tag) {}

VALUE_EQUALITY_TEMPLATE
const AML::value* VALUE_EQUALITY::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool equal = args[0] == args[1];
    return make_value_.make(equal ? true_tag_ : false_tag_, {});
}

#undef VALUE_EQUALITY
#undef VALUE_EQUALITY_TEMPLATE

#endif
