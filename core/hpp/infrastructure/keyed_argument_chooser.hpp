#ifndef KEYED_ARGUMENT_CHOOSER_HPP
#define KEYED_ARGUMENT_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "debug_assert.hpp"

namespace AML {

// Chooses a primitive's arguments one position at a time, each from the operands that still leave a witness, by a key
// measured for each operand (how recent it is, for instance), so the choice among them can follow the keys.
template<typename IFindArgumentOptions, typename IMeasureKey, typename IChooseOption>
struct keyed_argument_chooser {
    keyed_argument_chooser(
        IFindArgumentOptions& find_argument_options, IMeasureKey& measure_key, IChooseOption& choose_option);
    std::vector<size_t> choose(prim_id prim, const std::vector<type_id>& arg_types);
private:
    IFindArgumentOptions& find_argument_options_;
    IMeasureKey& measure_key_;
    IChooseOption& choose_option_;
};

#define KEYED_ARGUMENT_CHOOSER_TEMPLATE \
    template<typename IFindArgumentOptions, typename IMeasureKey, typename IChooseOption>
#define KEYED_ARGUMENT_CHOOSER          keyed_argument_chooser<IFindArgumentOptions, IMeasureKey, IChooseOption>

KEYED_ARGUMENT_CHOOSER_TEMPLATE
KEYED_ARGUMENT_CHOOSER::keyed_argument_chooser(
    IFindArgumentOptions& find_argument_options, IMeasureKey& measure_key, IChooseOption& choose_option)
    : find_argument_options_(find_argument_options)
    , measure_key_(measure_key)
    , choose_option_(choose_option) {}

KEYED_ARGUMENT_CHOOSER_TEMPLATE
std::vector<size_t> KEYED_ARGUMENT_CHOOSER::choose(prim_id prim, const std::vector<type_id>& arg_types) {
    std::vector<size_t> args;
    while (args.size() < arg_types.size()) {
        const std::vector<size_t> options = find_argument_options_.find(prim, arg_types, args);
        DEBUG_ASSERT(!options.empty());
        std::vector<size_t> keys;
        for (const size_t operand : options)
            keys.push_back(measure_key_.measure(operand));
        args.push_back(options.at(choose_option_.choose(keys)));
    }
    return args;
}

#undef KEYED_ARGUMENT_CHOOSER
#undef KEYED_ARGUMENT_CHOOSER_TEMPLATE

}

#endif
