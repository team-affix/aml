#include "infrastructure/obsidian_manifest.hpp"

namespace AML {

namespace {

const mcts_tree_node_id k_root = 0;
const double k_best_weight = 1.0;

}

obsidian_manifest::obsidian_manifest(
    const native_bin& training,
    const std::vector<native_constant>& constants,
    const std::vector<native_type>& types,
    const std::vector<native_primitive>& primitives,
    const std::vector<feature_id>& root,
    const obsidian_options& options)
    : random_(options.seed)
    , row_key_numberer_()
    , uniform_column_detector_()
    , native_types_(types)
    , native_primitives_(primitives)
    , argument_types_(primitives)
    , label_classes_(native_types_.type(training.labels.type).classify(training.labels.cells))
    , feature_graph_()
    , constant_graph_()
    , derivation_index_()
    , column_hash_index_()
    , column_cache_(options.column_cache_bytes)
    , constant_cache_(options.constant_cache_bytes)
    , best_table_tracker_()
    , visits_()
    , best_rewards_()
    , reward_bounds_()
    , in_rollout_()
    , tree_walker_(in_rollout_)
    , rollout_(random_)
    , exploration_constant_(options.exploration_constant)
    , value_delta_()
    , value_update_(visits_, best_rewards_, best_rewards_, value_delta_)
    , value_view_(visits_, best_rewards_, best_rewards_, k_best_weight)
    , cursor_(k_root)
    , backprop_path_(k_root)
    , visit_creditor_(backprop_path_, visits_, visits_)
    , value_creditor_(visit_creditor_, backprop_path_, value_update_)
    , policy_(visits_, value_view_, reward_bounds_, reward_bounds_, tree_walker_, exploration_constant_)
    , mcts_chooser_(
          visits_, tree_walker_, policy_, rollout_, cursor_, cursor_, backprop_path_, in_rollout_, in_rollout_)
    , terminator_(backprop_path_, value_creditor_, backprop_path_, backprop_path_, cursor_, in_rollout_, k_root)
    , option_chooser_(mcts_chooser_)
    , column_computer_(native_primitives_, training.inputs, training.row_count)
    , cached_column_maker_(native_types_)
    , column_byte_measurer_(native_types_)
    , constant_computer_(native_primitives_, native_types_, constants)
    , constant_byte_measurer_(native_types_)
    , constant_reader_(
          constant_graph_, constant_cache_, constant_computer_, constant_byte_measurer_, constant_cache_)
    , column_reader_(
          feature_graph_, constant_reader_, column_cache_, column_computer_, cached_column_maker_,
          column_byte_measurer_, column_cache_)
    , ancestry_finder_(feature_graph_)
    , constant_ancestry_finder_(constant_graph_)
    , ancestry_sizer_(ancestry_finder_, feature_graph_, constant_ancestry_finder_)
    , distinct_row_counter_(column_reader_, row_key_numberer_, training.row_count)
    , droppable_member_finder_(column_reader_, row_key_numberer_, label_classes_)
    , key_drawer_(random_)
    , equal_feature_finder_(column_hash_index_, column_reader_, native_types_)
    , origin_replacer_(ancestry_sizer_, feature_graph_)
    , feature_recorder_(
          key_drawer_, feature_graph_, derivation_index_, column_hash_index_, column_byte_measurer_, column_cache_)
    , feature_deriver_(
          derivation_index_, column_reader_, constant_reader_, column_computer_, native_primitives_,
          cached_column_maker_, uniform_column_detector_, equal_feature_finder_, origin_replacer_, derivation_index_,
          feature_recorder_)
    , operand_pool_finder_(feature_graph_)
    , eligible_primitive_finder_(argument_types_)
    , fillable_primitive_finder_(native_primitives_, constant_graph_)
    , application_chooser_(native_primitives_, constant_graph_, option_chooser_)
    , set_mover_(
          droppable_member_finder_, ancestry_finder_, operand_pool_finder_, eligible_primitive_finder_,
          fillable_primitive_finder_, option_chooser_, application_chooser_, feature_deriver_)
    , set_cost_measurer_(ancestry_sizer_, distinct_row_counter_)
    , model_graph_builder_(feature_graph_)
    , table_model_builder_(
          model_graph_builder_, column_reader_, native_types_, training.labels, training.row_count)
    , set_walk_episode_(
          set_mover_, set_cost_measurer_, table_model_builder_, best_table_tracker_, root, options.max_moves)
    , mcts_walk_episode_(set_walk_episode_, value_delta_, terminator_)
    , input_feature_seeder_(column_computer_, cached_column_maker_, feature_recorder_, uniform_column_detector_)
    , constant_atom_seeder_(constant_graph_)
    , graph_row_evaluator_(native_primitives_, native_types_, constant_reader_)
    , value_class_finder_(native_types_)
    , table_model_evaluator_(graph_row_evaluator_, value_class_finder_) {}

}
