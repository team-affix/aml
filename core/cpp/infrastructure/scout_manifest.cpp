#include "infrastructure/scout_manifest.hpp"

scout_manifest::scout_manifest(
    schema& types, value_pool& values, const std::vector<primitive>& primitives, const scout_options& options)
    : schema_(types)
    , values_(values)
    , features_()
    , nodes_()
    , constants_()
    , feature_candidates_()
    , applications_()
    , uniform_value_finder_()
    , value_sizer_()
    , feature_sizer_(value_sizer_)
    , model_sizer_(feature_sizer_, value_sizer_)
    , primitive_dispatcher_(primitives)
    , variable_candidate_finder_(features_, feature_sizer_, uniform_value_finder_)
    , constant_enumerator_(schema_, schema_, values_)
    , constant_seeder_(schema_, constant_enumerator_, features_, constants_)
    , operand_table_(constants_, constants_, feature_candidates_, feature_candidates_)
    , operand_applier_(operand_table_, primitive_dispatcher_)
    , app_candidate_finder_(operand_applier_, uniform_value_finder_, operand_table_, features_, feature_sizer_)
    , witness_detector_(operand_table_, operand_table_, operand_table_, applications_)
    , primitive_option_finder_(primitive_dispatcher_, primitive_dispatcher_, witness_detector_)
    , argument_option_finder_(operand_table_, operand_table_, witness_detector_)
    , option_chooser_()
    , argument_chooser_(argument_option_finder_, option_chooser_)
    , primitive_applier_(
          primitive_dispatcher_, argument_chooser_, applications_, app_candidate_finder_, feature_candidates_)
    , synthesis_starter_(feature_candidates_, applications_, variable_candidate_finder_, feature_candidates_)
    , feature_synthesizer_(
          synthesis_starter_, applications_, primitive_option_finder_, option_chooser_, primitive_applier_,
          feature_candidates_, options.max_applications)
    , leaf_detector_(uniform_value_finder_)
    , bin_splitter_(schema_, schema_)
    , model_builder_(
          leaf_detector_, feature_synthesizer_, option_chooser_, bin_splitter_, variable_candidate_finder_,
          option_chooser_, nodes_, nodes_, nodes_)
    , best_model_tracker_()
    , model_episode_(model_builder_, model_sizer_, best_model_tracker_)
    , feature_evaluator_(primitive_dispatcher_)
    , model_evaluator_(feature_evaluator_, schema_) {}
