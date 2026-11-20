#include "infrastructure/scout_manifest.hpp"

namespace AML {

scout_manifest::scout_manifest(
    schema& types,
    const std::vector<typed_primitive>& primitives,
    const scout_options& options)
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
    , option_chooser_()
    , argument_chooser_(argument_option_finder_, option_chooser_)
    , primitive_applier_(
          primitive_store_, argument_chooser_, applications_, app_candidate_finder_, feature_candidates_)
    , synthesis_starter_(feature_candidates_, applications_, variable_candidate_finder_, feature_candidates_)
    , feature_synthesizer_(
          synthesis_starter_, applications_, primitive_option_finder_, option_chooser_, option_chooser_,
          primitive_applier_, feature_candidates_, output_finder_, options.max_applications)
    , leaf_detector_(uniform_value_finder_)
    , bin_splitter_(schema_)
    , bound_argument_chooser_(variable_candidate_finder_, option_chooser_)
    , model_builder_(
          leaf_detector_, feature_synthesizer_, output_finder_, option_chooser_, bound_argument_chooser_,
          bin_splitter_)
    , best_model_tracker_()
    , model_episode_(model_builder_, model_sizer_, best_model_tracker_)
    , feature_evaluator_(primitive_caller_)
    , model_evaluator_(feature_evaluator_) {}

}
