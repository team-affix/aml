#include "support/bool_or.hpp"
#include "debug_assert.hpp"

bool_or::bool_or(AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value bool_or::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const bool left = args[0]->ctor == true_ctor_;
    const bool right = args[1]->ctor == true_ctor_;
    const bool result = left || right;
    return AML::value{.ctor = result ? true_ctor_ : false_ctor_, .args = {}};
}
