#include "probe/list_primitives.hpp"
#include "debug_assert.hpp"

list_is_nil::list_is_nil(AML::ctor_id nil_ctor, AML::ctor_id false_ctor, AML::ctor_id true_ctor)
    : nil_ctor_(nil_ctor)
    , false_ctor_(false_ctor)
    , true_ctor_(true_ctor) {}

AML::value list_is_nil::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    const bool empty = args[0]->ctor == nil_ctor_;
    return AML::value{.ctor = empty ? true_ctor_ : false_ctor_, .args = {}};
}

list_head::list_head(AML::ctor_id nil_ctor, AML::value fallback)
    : nil_ctor_(nil_ctor)
    , fallback_(fallback) {}

AML::value list_head::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    if (args[0]->ctor == nil_ctor_)
        return fallback_;
    return args[0]->args[0];
}

list_tail::list_tail(AML::ctor_id nil_ctor)
    : nil_ctor_(nil_ctor) {}

AML::value list_tail::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 1);
    if (args[0]->ctor == nil_ctor_)
        return *args[0];
    return args[0]->args[1];
}
