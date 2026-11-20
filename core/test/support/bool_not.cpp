#include "support/bool_not.hpp"
#include "debug_assert.hpp"

bool_not::bool_not(AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value bool_not::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool operand = args[0]->ctor == true_ctor_;
    return AML::value{.ctor = operand ? false_ctor_ : true_ctor_, .args = {}};
}
