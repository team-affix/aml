#include "probe/list_tail.hpp"

#include "debug_assert.hpp"

list_tail::list_tail(AML::ctor_id nil_ctor)
    : nil_ctor_(nil_ctor) {}

AML::value list_tail::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    if (args[0]->ctor == nil_ctor_)
        return *args[0];
    return args[0]->args[1];
}
