#ifndef CONSTANT_ATOM_SEEDER_HPP
#define CONSTANT_ATOM_SEEDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/constant_derivation.hpp"
#include "value_objects/constant_record.hpp"
#include "value_objects/native_constant.hpp"

namespace AML {

// Seeds the constant graph with every offered constant as an atom, by its position among the offered constants, in
// order. Its value is read from the offered constants when it is computed.
template<typename IInsertConstantRecord>
struct constant_atom_seeder {
    constant_atom_seeder(IInsertConstantRecord& insert_constant_record);
    void seed(const std::vector<native_constant>& constants);
private:
    IInsertConstantRecord& insert_constant_record_;
};

#define CONSTANT_ATOM_SEEDER_TEMPLATE template<typename IInsertConstantRecord>
#define CONSTANT_ATOM_SEEDER          constant_atom_seeder<IInsertConstantRecord>

CONSTANT_ATOM_SEEDER_TEMPLATE
CONSTANT_ATOM_SEEDER::constant_atom_seeder(IInsertConstantRecord& insert_constant_record)
    : insert_constant_record_(insert_constant_record) {}

CONSTANT_ATOM_SEEDER_TEMPLATE
void CONSTANT_ATOM_SEEDER::seed(const std::vector<native_constant>& constants) {
    for (size_t index = 0; index < constants.size(); ++index) {
        insert_constant_record_.insert(constant_record{
            .origin = constant_derivation{constant_derivation::atom{.index = index}},
            .type = constants[index].type});
    }
}

#undef CONSTANT_ATOM_SEEDER
#undef CONSTANT_ATOM_SEEDER_TEMPLATE

}

#endif
