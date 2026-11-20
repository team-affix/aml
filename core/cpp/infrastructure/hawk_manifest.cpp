#include "infrastructure/hawk_manifest.hpp"

namespace AML {

namespace {

const mcts_tree_node_id k_root = 0;

}

hawk_manifest::hawk_manifest(
    schema& types,
    const std::vector<typed_primitive>& primitives,
    const hawk_options& options)
    : schema_(types)
    , constants_()
    , feature_candidates_()
    , applications_()
    , output_finder_()
    , uniform_value_finder_()
    , value_sizer_()
    , feature_sizer_(value_sizer_)
    , model_sizer_(feature_sizer_)
    , primitive_store_(primitives)
    , primitive_caller_(primitive_store_)
    , variable_candidate_finder_(feature_sizer_, uniform_value_finder_)
    , constant_seeder_(constants_)
    , operand_table_(constants_, constants_, feature_candidates_, feature_candidates_)
    , operand_applier_(operand_table_, primitive_caller_)
    , app_candidate_finder_(operand_applier_, uniform_value_finder_, operand_table_, feature_sizer_)
    , witness_detector_(operand_table_, operand_table_, operand_table_, applications_)
    , primitive_option_finder_(primitive_store_, primitive_store_, witness_detector_)
    , argument_option_finder_(operand_table_, operand_table_, witness_detector_)
    , visits_()
    , rewards_()
    , reward_bounds_()
    , random_(options.seed)
    , in_rollout_()
    , tree_walker_(in_rollout_)
    , rollout_(random_)
    , choice_log_()
    , rollout_logger_(rollout_, choice_log_)
    , exploration_constant_(options.exploration_constant)
    , value_delta_()
    , value_update_(rewards_, rewards_, value_delta_)
    , cursor_(k_root)
    , backprop_path_(k_root)
    , visit_creditor_(backprop_path_, visits_, visits_)
    , value_creditor_(visit_creditor_, backprop_path_, value_update_)
    , policy_(visits_, rewards_, reward_bounds_, reward_bounds_, tree_walker_, exploration_constant_)
    , mcts_chooser_(
          visits_, tree_walker_, policy_, rollout_logger_, cursor_, cursor_, backprop_path_, in_rollout_, in_rollout_)
    , grafter_(in_rollout_, value_delta_, choice_log_, choice_log_, backprop_path_, tree_walker_, backprop_path_)
    , backprop_terminator_(
          backprop_path_, value_creditor_, backprop_path_, backprop_path_, cursor_, in_rollout_, k_root)
    , terminator_(grafter_, backprop_terminator_)
    , option_chooser_(mcts_chooser_)
    , binary_option_chooser_(option_chooser_)
    , argument_chooser_(argument_option_finder_, binary_option_chooser_)
    , primitive_applier_(
          primitive_store_, argument_chooser_, applications_, app_candidate_finder_, feature_candidates_)
    , synthesis_starter_(feature_candidates_, applications_, variable_candidate_finder_, feature_candidates_)
    , feature_synthesizer_(
          synthesis_starter_, applications_, primitive_option_finder_, binary_option_chooser_, binary_option_chooser_,
          primitive_applier_, feature_candidates_, output_finder_, options.max_applications)
    , leaf_detector_(uniform_value_finder_)
    , bin_splitter_(schema_)
    , bound_argument_chooser_(variable_candidate_finder_, binary_option_chooser_)
    , model_builder_(
          leaf_detector_, feature_synthesizer_, output_finder_, binary_option_chooser_, bound_argument_chooser_,
          bin_splitter_)
    , best_model_tracker_()
    , model_episode_(model_builder_, model_sizer_, best_model_tracker_)
    , mcts_episode_(model_episode_, value_delta_, terminator_)
    , feature_evaluator_(primitive_caller_)
    , model_evaluator_(feature_evaluator_) {}

}
