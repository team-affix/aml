#ifndef BOOL_NOT_HPP
#define BOOL_NOT_HPP

#include <cstddef>
#include <vector>
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean negation over a user boolean type with the constructors at tags `false_tag` and `true_tag`.
template<typename IMakeValue>
struct bool_not {
    bool_not(IMakeValue& make_value, size_t false_tag, size_t true_tag);
    const value* operator()(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    size_t false_tag_;
    size_t true_tag_;
};

#define BOOL_NOT_TEMPLATE template<typename IMakeValue>
#define BOOL_NOT          bool_not<IMakeValue>

BOOL_NOT_TEMPLATE
BOOL_NOT::bool_not(IMakeValue& make_value, size_t false_tag, size_t true_tag)
    : make_value_(make_value)
    , false_tag_(false_tag)
    , true_tag_(true_tag) {}

BOOL_NOT_TEMPLATE
const value* BOOL_NOT::operator()(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool operand = args[0]->tag == true_tag_;
    return make_value_.make(operand ? false_tag_ : true_tag_, {});
}

#undef BOOL_NOT
#undef BOOL_NOT_TEMPLATE

#endif
