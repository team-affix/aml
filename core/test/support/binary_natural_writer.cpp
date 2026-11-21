#include "support/binary_natural_writer.hpp"

binary_natural_writer::binary_natural_writer(binary_natural_ctors naturals)
    : naturals_(naturals) {}

AML::value binary_natural_writer::operator()(unsigned __int128 number) const {
    const bool too_large = number > k_binary_natural_largest;
    const size_t capped = too_large ? k_binary_natural_largest : static_cast<size_t>(number);
    if (capped == 0)
        return AML::value{.ctor = naturals_.zero, .args = {}};
    const bool odd = capped % 2 == 1;
    return AML::value{.ctor = odd ? naturals_.twice_plus_one : naturals_.twice, .args = {(*this)(capped / 2)}};
}
