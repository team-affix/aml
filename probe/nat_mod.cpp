#include "probe/nat_mod.hpp"
#include <cstddef>
#include <utility>
#include "debug_assert.hpp"

namespace {

size_t count_successors(const AML::value& number, AML::ctor_id suc_ctor) {
    size_t count = 0;
    for (const AML::value* rest = &number; rest->ctor == suc_ctor; rest = &rest->args[0])
        ++count;
    return count;
}

}

nat_mod::nat_mod(AML::ctor_id zero_ctor, AML::ctor_id suc_ctor)
    : zero_ctor_(zero_ctor)
    , suc_ctor_(suc_ctor) {}

AML::value nat_mod::operator()(const std::vector<const AML::value*>& args) const {
    DEBUG_ASSERT(args.size() == 2);
    const size_t divisor = count_successors(*args[1], suc_ctor_);
    if (divisor == 0)
        return *args[0];
    const size_t remainder = count_successors(*args[0], suc_ctor_) % divisor;
    AML::value result{.ctor = zero_ctor_, .args = {}};
    for (size_t step = 0; step < remainder; ++step)
        result = AML::value{.ctor = suc_ctor_, .args = {std::move(result)}};
    return result;
}
