#include "value_objects/derivation_hash.hpp"

#include <functional>
#include <variant>

namespace AML {

size_t derivation_hash::operator()(const derivation& origin) const noexcept {
    const size_t seed = std::hash<size_t>{}(origin.content.index());
    if (const derivation::input* input = std::get_if<derivation::input>(&origin.content))
        return hash_combine(seed, std::hash<var_id>{}(input->index));
    const derivation::application& application = std::get<derivation::application>(origin.content);
    size_t combined = hash_combine(seed, std::hash<prim_id>{}(application.prim));
    for (const operand& arg : application.args) {
        combined = hash_combine(combined, std::hash<size_t>{}(arg.source.index()));
        const operand::from_feature* feature = std::get_if<operand::from_feature>(&arg.source);
        const size_t id = feature != nullptr ? feature->id : std::get<operand::from_constant>(arg.source).id;
        combined = hash_combine(combined, std::hash<size_t>{}(id));
    }
    return combined;
}

size_t derivation_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

}
