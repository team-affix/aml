#include "probe/list_length.hpp"

#include <utility>
#include "debug_assert.hpp"

list_length::list_length(AML::ctor_id nil_ctor, AML::ctor_id zero_ctor, AML::ctor_id suc_ctor)
    : nil_ctor_(nil_ctor)
    , zero_ctor_(zero_ctor)
    , suc_ctor_(suc_ctor) {}

AML::value list_length::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    AML::value length{.ctor = zero_ctor_, .args = {}};
    for (const AML::value* rest = args[0]; rest->ctor != nil_ctor_; rest = &rest->args[1])
        length = AML::value{.ctor = suc_ctor_, .args = {std::move(length)}};
    return length;
}
