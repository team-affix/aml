#ifndef BOOL_OR_HPP
#define BOOL_OR_HPP

#include <cstddef>
#include <vector>
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean disjunction over a user boolean type with the constructors at tags `false_tag` and `true_tag`.
template<typename IMakeValue>
struct bool_or {
    bool_or(IMakeValue& make_value, size_t false_tag, size_t true_tag);
    const value* operator()(const std::vector<const value*>& args) const;
private:
    IMakeValue& make_value_;
    size_t false_tag_;
    size_t true_tag_;
};

#define BOOL_OR_TEMPLATE template<typename IMakeValue>
#define BOOL_OR          bool_or<IMakeValue>

BOOL_OR_TEMPLATE
BOOL_OR::bool_or(IMakeValue& make_value, size_t false_tag, size_t true_tag)
    : make_value_(make_value)
    , false_tag_(false_tag)
    , true_tag_(true_tag) {}

BOOL_OR_TEMPLATE
const value* BOOL_OR::operator()(const std::vector<const value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool left = args[0]->tag == true_tag_;
    const bool right = args[1]->tag == true_tag_;
    const bool result = left || right;
    return make_value_.make(result ? true_tag_ : false_tag_, {});
}

#undef BOOL_OR
#undef BOOL_OR_TEMPLATE

#endif
