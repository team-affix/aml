#ifndef CHARACTER_TRANSPILER_HPP
#define CHARACTER_TRANSPILER_HPP

#include <cstdint>
#include "value_objects/aml_expr.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/nat_format.hpp"

template<typename ITranspileNat>
struct character_transpiler {
    explicit character_transpiler(ITranspileNat& transpile_nat);

    const lc_expr* transpile_character(const aml_expr::character& c);

private:
    ITranspileNat& transpile_nat_;
};

template<typename IN>
character_transpiler<IN>::character_transpiler(IN& transpile_nat) : transpile_nat_(transpile_nat) {}

template<typename IN>
const lc_expr* character_transpiler<IN>::transpile_character(const aml_expr::character& c) {
    return transpile_nat_.transpile_nat(
        {static_cast<uint64_t>(static_cast<unsigned char>(c.value)), nat_format::scott});
}

#endif
