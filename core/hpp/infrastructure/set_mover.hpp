#ifndef SET_MOVER_HPP
#define SET_MOVER_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/prim_id.hpp"

namespace AML {

// Moves a set, its members in increasing id, by one drop or one addition. Drops are offered while some member can be
// dropped with the set still a function, additions while some primitive is fillable over the set's ancestry; with
// both offered the first choice takes a drop (option 0) or an addition (option 1), with one it is taken without a
// choice. A drop chooses among the droppable members. An addition chooses an application and adds its feature; one
// that derives no feature, or a member, leaves the set as it was. There is no move when neither is offered.
template<
    typename IFindDroppableMembers, typename IFindAncestry, typename IFindOperandPool,
    typename IFindEligiblePrimitives, typename IFindFillablePrimitives, typename IChooseOption,
    typename IChooseApplication, typename IDeriveFeature>
struct set_mover {
    set_mover(
        IFindDroppableMembers& find_droppable_members,
        IFindAncestry& find_ancestry,
        IFindOperandPool& find_operand_pool,
        IFindEligiblePrimitives& find_eligible_primitives,
        IFindFillablePrimitives& find_fillable_primitives,
        IChooseOption& choose_option,
        IChooseApplication& choose_application,
        IDeriveFeature& derive_feature);
    std::optional<std::vector<feature_id>> move(const std::vector<feature_id>& members);
private:
    IFindDroppableMembers& find_droppable_members_;
    IFindAncestry& find_ancestry_;
    IFindOperandPool& find_operand_pool_;
    IFindEligiblePrimitives& find_eligible_primitives_;
    IFindFillablePrimitives& find_fillable_primitives_;
    IChooseOption& choose_option_;
    IChooseApplication& choose_application_;
    IDeriveFeature& derive_feature_;
};

#define SET_MOVER_TEMPLATE \
    template< \
        typename IFindDroppableMembers, typename IFindAncestry, typename IFindOperandPool, \
        typename IFindEligiblePrimitives, typename IFindFillablePrimitives, typename IChooseOption, \
        typename IChooseApplication, typename IDeriveFeature>
#define SET_MOVER \
    set_mover< \
        IFindDroppableMembers, IFindAncestry, IFindOperandPool, IFindEligiblePrimitives, IFindFillablePrimitives, \
        IChooseOption, IChooseApplication, IDeriveFeature>

SET_MOVER_TEMPLATE
SET_MOVER::set_mover(
    IFindDroppableMembers& find_droppable_members,
    IFindAncestry& find_ancestry,
    IFindOperandPool& find_operand_pool,
    IFindEligiblePrimitives& find_eligible_primitives,
    IFindFillablePrimitives& find_fillable_primitives,
    IChooseOption& choose_option,
    IChooseApplication& choose_application,
    IDeriveFeature& derive_feature)
    : find_droppable_members_(find_droppable_members)
    , find_ancestry_(find_ancestry)
    , find_operand_pool_(find_operand_pool)
    , find_eligible_primitives_(find_eligible_primitives)
    , find_fillable_primitives_(find_fillable_primitives)
    , choose_option_(choose_option)
    , choose_application_(choose_application)
    , derive_feature_(derive_feature) {}

SET_MOVER_TEMPLATE
std::optional<std::vector<feature_id>> SET_MOVER::move(const std::vector<feature_id>& members) {
    const std::vector<size_t> droppable = find_droppable_members_.find(members);
    const std::unordered_map<std::type_index, std::vector<feature_id>> pool =
        find_operand_pool_.find(find_ancestry_.find(members));
    const std::vector<prim_id> fillable = find_fillable_primitives_.find(pool, find_eligible_primitives_.find(pool));
    const bool can_drop = !droppable.empty();
    const bool can_add = !fillable.empty();
    if (!can_drop && !can_add)
        return std::nullopt;
    const bool dropping = can_drop && (!can_add || choose_option_.choose(2) == 0);
    std::vector<feature_id> moved = members;
    if (dropping) {
        const size_t position = droppable[choose_option_.choose(droppable.size())];
        moved.erase(moved.begin() + static_cast<std::ptrdiff_t>(position));
        return moved;
    }
    const std::optional<feature_id> derived = derive_feature_.derive(choose_application_.choose(pool, fillable));
    if (!derived.has_value())
        return moved;
    const auto place = std::lower_bound(moved.begin(), moved.end(), derived.value());
    const bool held = place != moved.end() && *place == derived.value();
    if (held)
        return moved;
    moved.insert(place, derived.value());
    return moved;
}

#undef SET_MOVER
#undef SET_MOVER_TEMPLATE

}

#endif
