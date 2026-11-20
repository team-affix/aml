#include "support/value_equality.hpp"
#include "debug_assert.hpp"

value_equality::value_equality(AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value value_equality::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool equal = *args[0] == *args[1];
    return AML::value{.ctor = equal ? true_ctor_ : false_ctor_, .args = {}};
}
