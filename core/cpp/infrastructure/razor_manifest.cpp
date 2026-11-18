#include "infrastructure/razor_manifest.hpp"

namespace {

const mcts_tree_node_id k_root = 0;

}

razor_manifest::razor_manifest(
    schema& types, value_pool& values, const std::vector<primitive>& primitives, const razor_options& options)
    : schema_(types)
    , values_(values)
    , features_()
    , nodes_()
    , constants_()
    , candidate_index_()
    , uniform_value_finder_()
    , value_sizer_()
    , feature_sizer_(value_sizer_)
    , model_sizer_(feature_sizer_, value_sizer_)
    , primitive_dispatcher_(primitives)
    , column_applier_(primitive_dispatcher_)
    , argument_tuple_enumerator_()
    , variable_candidate_finder_(features_, feature_sizer_, uniform_value_finder_)
    , constant_enumerator_(schema_, schema_, values_)
    , constant_seeder_(schema_, constant_enumerator_, features_, constants_)
    , constant_candidate_finder_(constants_, feature_sizer_)
    , app_candidate_finder_(column_applier_, uniform_value_finder_, features_, feature_sizer_)
    , feature_synthesizer_(
          candidate_index_, candidate_index_, candidate_index_, variable_candidate_finder_, constant_candidate_finder_,
          primitive_dispatcher_, primitive_dispatcher_, argument_tuple_enumerator_, app_candidate_finder_,
          options.max_synthesis_depth)
    , leaf_detector_(uniform_value_finder_)
    , bin_splitter_(schema_, schema_)
    , visits_()
    , rewards_()
    , random_(options.seed)
    , tree_walker_()
    , rollout_(random_)
    , exploration_constant_(options.exploration_constant)
    , value_delta_()
    , value_update_(rewards_, rewards_, value_delta_)
    , cursor_(k_root)
    , backprop_path_(k_root)
    , visit_creditor_(backprop_path_, visits_, visits_)
    , value_creditor_(visit_creditor_, backprop_path_, value_update_)
    , policy_(visits_, rewards_, tree_walker_, exploration_constant_)
    , in_rollout_()
    , mcts_chooser_(
          visits_, tree_walker_, policy_, rollout_, cursor_, cursor_, backprop_path_, in_rollout_, in_rollout_)
    , terminator_(backprop_path_, value_creditor_, backprop_path_, backprop_path_, cursor_, in_rollout_, k_root)
    , candidate_chooser_(mcts_chooser_)
    , model_builder_(
          leaf_detector_, feature_synthesizer_, candidate_chooser_, bin_splitter_, variable_candidate_finder_,
          candidate_chooser_, nodes_, nodes_, nodes_)
    , best_model_tracker_()
    , model_episode_(model_builder_, model_sizer_, best_model_tracker_)
    , mcts_episode_(model_episode_, value_delta_, terminator_)
    , feature_evaluator_(primitive_dispatcher_)
    , model_evaluator_(feature_evaluator_, schema_) {}
