#include "support/nat_add.hpp"
#include <utility>
#include "debug_assert.hpp"

nat_add::nat_add(AML::ctor_id suc_ctor)
    : suc_ctor_(suc_ctor) {}

AML::value nat_add::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    AML::value sum = *args[0];
    for (const AML::value* rest = args[1]; rest->ctor == suc_ctor_; rest = &rest->args[0])
        sum = AML::value{.ctor = suc_ctor_, .args = {std::move(sum)}};
    return sum;
}
