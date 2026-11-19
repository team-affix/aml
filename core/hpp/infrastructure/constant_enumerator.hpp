#ifndef CONSTANT_ENUMERATOR_HPP
#define CONSTANT_ENUMERATOR_HPP

#include <cstddef>
#include <vector>
#include "value_objects/ctor_decl.hpp"
#include "value_objects/ctor_id.hpp"
#include "value_objects/type_id.hpp"
#include "value_objects/value.hpp"

// Enumerates every value of a type up to a size, smallest first.
template<typename IGetTypeCtors, typename IGetCtorDecl, typename IMakeValue>
struct constant_enumerator {
    constant_enumerator(IGetTypeCtors& get_type_ctors, IGetCtorDecl& get_ctor_decl, IMakeValue& make_value);
    std::vector<const value*> enumerate(type_id type, size_t max_size);
private:
    std::vector<const value*> enumerate_exact(type_id type, size_t size);
    std::vector<std::vector<const value*>> enumerate_args(
        const std::vector<type_id>& arg_types, size_t first_argument, size_t size);

    IGetTypeCtors& get_type_ctors_;
    IGetCtorDecl& get_ctor_decl_;
    IMakeValue& make_value_;
};

#define CONSTANT_ENUMERATOR_TEMPLATE template<typename IGetTypeCtors, typename IGetCtorDecl, typename IMakeValue>
#define CONSTANT_ENUMERATOR          constant_enumerator<IGetTypeCtors, IGetCtorDecl, IMakeValue>

CONSTANT_ENUMERATOR_TEMPLATE
CONSTANT_ENUMERATOR::constant_enumerator(
    IGetTypeCtors& get_type_ctors, IGetCtorDecl& get_ctor_decl, IMakeValue& make_value)
    : get_type_ctors_(get_type_ctors)
    , get_ctor_decl_(get_ctor_decl)
    , make_value_(make_value) {}

CONSTANT_ENUMERATOR_TEMPLATE
std::vector<const value*> CONSTANT_ENUMERATOR::enumerate(type_id type, size_t max_size) {
    std::vector<const value*> values;
    for (size_t size = 1; size <= max_size; ++size) {
        const std::vector<const value*> sized = enumerate_exact(type, size);
        values.insert(values.end(), sized.begin(), sized.end());
    }
    return values;
}

CONSTANT_ENUMERATOR_TEMPLATE
std::vector<const value*> CONSTANT_ENUMERATOR::enumerate_exact(type_id type, size_t size) {
    std::vector<const value*> values;
    for (const ctor_id ctor : get_type_ctors_.ctors(type)) {
        const ctor_decl& decl = get_ctor_decl_.decl(ctor);
        for (const std::vector<const value*>& arguments : enumerate_args(decl.arg_types, 0, size - 1))
            values.push_back(make_value_.make(ctor, arguments));
    }
    return values;
}

CONSTANT_ENUMERATOR_TEMPLATE
std::vector<std::vector<const value*>> CONSTANT_ENUMERATOR::enumerate_args(
    const std::vector<type_id>& arg_types, size_t first_argument, size_t size) {
    std::vector<std::vector<const value*>> combinations;
    const size_t remaining_arguments = arg_types.size() - first_argument;
    if (remaining_arguments == 0) {
        if (size == 0)
            combinations.emplace_back();
        return combinations;
    }
    if (size < remaining_arguments)
        return combinations;
    const size_t largest_first = size - (remaining_arguments - 1);
    for (size_t first_size = 1; first_size <= largest_first; ++first_size) {
        const std::vector<std::vector<const value*>> rests =
            enumerate_args(arg_types, first_argument + 1, size - first_size);
        if (rests.empty())
            continue;
        for (const value* first : enumerate_exact(arg_types[first_argument], first_size)) {
            for (const std::vector<const value*>& rest : rests) {
                std::vector<const value*> combination{first};
                combination.insert(combination.end(), rest.begin(), rest.end());
                combinations.push_back(combination);
            }
        }
    }
    return combinations;
}

#undef CONSTANT_ENUMERATOR
#undef CONSTANT_ENUMERATOR_TEMPLATE

#endif
