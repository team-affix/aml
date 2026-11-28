#include "support/binary_natural_reader.hpp"

binary_natural_reader::binary_natural_reader(binary_natural_ctors naturals)
    : naturals_(naturals) {}

size_t binary_natural_reader::operator()(const AML::value& term) const {
    if (term.ctor == naturals_.zero)
        return 0;
    const size_t low_bit = term.ctor == naturals_.twice_plus_one ? 1 : 0;
    return 2 * (*this)(term.args[0]) + low_bit;
}
