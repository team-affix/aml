#ifndef ARGUMENT_CHOOSER_HPP
#define ARGUMENT_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/prim_id.hpp"
#include "value_objects/type_id.hpp"
#include "debug_assert.hpp"

// Chooses a primitive's arguments one position at a time, each from the operands that still leave a witness.
template<typename IFindArgumentOptions, typename IChooseArgument>
struct argument_chooser {
    argument_chooser(IFindArgumentOptions& find_argument_options, IChooseArgument& choose_argument);
    std::vector<size_t> choose(prim_id prim, const std::vector<type_id>& arg_types);
private:
    IFindArgumentOptions& find_argument_options_;
    IChooseArgument& choose_argument_;
};

#define ARGUMENT_CHOOSER_TEMPLATE template<typename IFindArgumentOptions, typename IChooseArgument>
#define ARGUMENT_CHOOSER          argument_chooser<IFindArgumentOptions, IChooseArgument>

ARGUMENT_CHOOSER_TEMPLATE
ARGUMENT_CHOOSER::argument_chooser(IFindArgumentOptions& find_argument_options, IChooseArgument& choose_argument)
    : find_argument_options_(find_argument_options)
    , choose_argument_(choose_argument) {}

ARGUMENT_CHOOSER_TEMPLATE
std::vector<size_t> ARGUMENT_CHOOSER::choose(prim_id prim, const std::vector<type_id>& arg_types) {
    std::vector<size_t> args;
    while (args.size() < arg_types.size()) {
        const std::vector<size_t> options = find_argument_options_.find(prim, arg_types, args);
        DEBUG_ASSERT(!options.empty());
        args.push_back(options.at(choose_argument_.choose(options.size())));
    }
    return args;
}

#undef ARGUMENT_CHOOSER
#undef ARGUMENT_CHOOSER_TEMPLATE

#endif
