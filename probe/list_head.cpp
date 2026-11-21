#include "probe/list_head.hpp"

#include "debug_assert.hpp"

list_head::list_head(AML::ctor_id nil_ctor, AML::value fallback)
    : nil_ctor_(nil_ctor)
    , fallback_(fallback) {}

AML::value list_head::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    if (args[0]->ctor == nil_ctor_)
        return fallback_;
    return args[0]->args[0];
}
