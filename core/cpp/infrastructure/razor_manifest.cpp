#include "infrastructure/razor_manifest.hpp"

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
    , feature_synthesizer_(
          candidate_index_, candidate_index_, candidate_index_, variable_candidate_finder_, constants_,
          primitive_dispatcher_, primitive_dispatcher_, argument_tuple_enumerator_, column_applier_,
          uniform_value_finder_, features_, feature_sizer_, options.max_synthesis_depth)
    , leaf_detector_(uniform_value_finder_)
    , bin_splitter_(schema_, schema_)
    , visits_()
    , rewards_()
    , random_(options.seed)
    , mcts_(visits_, visits_, rewards_, rewards_, random_, options.exploration_constant, 0)
    , candidate_chooser_(mcts_.chooser)
    , model_builder_(
          leaf_detector_, feature_synthesizer_, candidate_chooser_, bin_splitter_, variable_candidate_finder_,
          candidate_chooser_, nodes_, nodes_, nodes_)
    , best_model_tracker_()
    , model_episode_(model_builder_, model_sizer_, best_model_tracker_)
    , mcts_episode_(model_episode_, mcts_.delta, mcts_.terminator)
    , feature_evaluator_(primitive_dispatcher_)
    , model_evaluator_(feature_evaluator_, schema_) {}
