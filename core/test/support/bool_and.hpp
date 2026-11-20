#ifndef BOOL_AND_HPP
#define BOOL_AND_HPP

#include <cstddef>
#include <vector>
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Boolean conjunction over a user boolean type with the constructors at tags `false_tag` and `true_tag`.
template<typename IMakeValue>
struct bool_and {
    bool_and(IMakeValue& make_value, size_t false_tag, size_t true_tag);
    const AML::value* operator()(const std::vector<const AML::value*>& args) const;
private:
    IMakeValue& make_value_;
    size_t false_tag_;
    size_t true_tag_;
};

#define BOOL_AND_TEMPLATE template<typename IMakeValue>
#define BOOL_AND          bool_and<IMakeValue>

BOOL_AND_TEMPLATE
BOOL_AND::bool_and(IMakeValue& make_value, size_t false_tag, size_t true_tag)
    : make_value_(make_value)
    , false_tag_(false_tag)
    , true_tag_(true_tag) {}

BOOL_AND_TEMPLATE
const AML::value* BOOL_AND::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool left = args[0]->tag == true_tag_;
    const bool right = args[1]->tag == true_tag_;
    const bool result = left && right;
    return make_value_.make(result ? true_tag_ : false_tag_, {});
}

#undef BOOL_AND
#undef BOOL_AND_TEMPLATE

#endif
