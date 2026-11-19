#ifndef SCOUT_MANIFEST_HPP
#define SCOUT_MANIFEST_HPP

#include <vector>
#include "infrastructure/app_candidate_finder.hpp"
#include "infrastructure/application_log.hpp"
#include "infrastructure/argument_chooser.hpp"
#include "infrastructure/argument_option_finder.hpp"
#include "infrastructure/best_model_tracker.hpp"
#include "infrastructure/bin_splitter.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/feature_synthesizer.hpp"
#include "infrastructure/bound_argument_chooser.hpp"
#include "infrastructure/first_option_chooser.hpp"
#include "infrastructure/leaf_detector.hpp"
#include "infrastructure/model_builder.hpp"
#include "infrastructure/model_episode.hpp"
#include "infrastructure/model_evaluator.hpp"
#include "infrastructure/model_sizer.hpp"
#include "infrastructure/operand_applier.hpp"
#include "infrastructure/operand_table.hpp"
#include "infrastructure/output_finder.hpp"
#include "infrastructure/primitive_applier.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_option_finder.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/synthesis_starter.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/value_pool.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "infrastructure/witness_detector.hpp"
#include "value_objects/scout_options.hpp"
#include "value_objects/typed_primitive.hpp"

// Scout: builds one model, taking the first candidate at every choice.
struct scout_manifest {
    struct feature_sizer_t : feature_sizer<value_sizer> {
        using feature_sizer<value_sizer>::feature_sizer;
    };
    struct model_sizer_t : model_sizer<feature_sizer_t> {
        using model_sizer<feature_sizer_t>::model_sizer;
    };
    struct primitive_caller_t : primitive_caller<primitive_store> {
        using primitive_caller<primitive_store>::primitive_caller;
    };
    struct variable_candidate_finder_t : variable_candidate_finder<feature_sizer_t, uniform_value_finder> {
        using variable_candidate_finder<feature_sizer_t, uniform_value_finder>::variable_candidate_finder;
    };
    struct constant_seeder_t : constant_seeder<constant_store> {
        using constant_seeder<constant_store>::constant_seeder;
    };
    struct operand_table_t
        : operand_table<constant_store, constant_store, cheapest_feature_index, cheapest_feature_index> {
        using operand_table<
            constant_store, constant_store, cheapest_feature_index, cheapest_feature_index>::operand_table;
    };
    struct operand_applier_t : operand_applier<operand_table_t, primitive_caller_t> {
        using operand_applier<operand_table_t, primitive_caller_t>::operand_applier;
    };
    struct app_candidate_finder_t
        : app_candidate_finder<operand_applier_t, uniform_value_finder, operand_table_t, feature_sizer_t> {
        using app_candidate_finder<
            operand_applier_t, uniform_value_finder, operand_table_t, feature_sizer_t>::app_candidate_finder;
    };
    struct witness_detector_t
        : witness_detector<operand_table_t, operand_table_t, operand_table_t, application_log> {
        using witness_detector<operand_table_t, operand_table_t, operand_table_t, application_log>::witness_detector;
    };
    struct primitive_option_finder_t
        : primitive_option_finder<primitive_store, primitive_store, witness_detector_t> {
        using primitive_option_finder<
            primitive_store, primitive_store, witness_detector_t>::primitive_option_finder;
    };
    struct argument_option_finder_t
        : argument_option_finder<operand_table_t, operand_table_t, witness_detector_t> {
        using argument_option_finder<operand_table_t, operand_table_t, witness_detector_t>::argument_option_finder;
    };
    struct argument_chooser_t : argument_chooser<argument_option_finder_t, first_option_chooser> {
        using argument_chooser<argument_option_finder_t, first_option_chooser>::argument_chooser;
    };
    struct primitive_applier_t : primitive_applier<
        primitive_store, argument_chooser_t, application_log, app_candidate_finder_t, cheapest_feature_index> {
        using primitive_applier<
            primitive_store, argument_chooser_t, application_log, app_candidate_finder_t,
            cheapest_feature_index>::primitive_applier;
    };
    struct synthesis_starter_t : synthesis_starter<
        cheapest_feature_index, application_log, variable_candidate_finder_t, cheapest_feature_index> {
        using synthesis_starter<
            cheapest_feature_index, application_log, variable_candidate_finder_t,
            cheapest_feature_index>::synthesis_starter;
    };
    struct feature_synthesizer_t : feature_synthesizer<
        synthesis_starter_t, application_log, primitive_option_finder_t, first_option_chooser, first_option_chooser,
        primitive_applier_t, cheapest_feature_index, output_finder> {
        using feature_synthesizer<
            synthesis_starter_t, application_log, primitive_option_finder_t, first_option_chooser, first_option_chooser,
            primitive_applier_t, cheapest_feature_index, output_finder>::feature_synthesizer;
    };
    struct leaf_detector_t : leaf_detector<uniform_value_finder> {
        using leaf_detector<uniform_value_finder>::leaf_detector;
    };
    struct bin_splitter_t : bin_splitter<schema> {
        using bin_splitter<schema>::bin_splitter;
    };
    struct bound_argument_chooser_t : bound_argument_chooser<variable_candidate_finder_t, first_option_chooser> {
        using bound_argument_chooser<variable_candidate_finder_t, first_option_chooser>::bound_argument_chooser;
    };
    struct model_builder_t : model_builder<
        leaf_detector_t, feature_synthesizer_t, output_finder, first_option_chooser, bound_argument_chooser_t,
        bin_splitter_t> {
        using model_builder<
            leaf_detector_t, feature_synthesizer_t, output_finder, first_option_chooser, bound_argument_chooser_t,
            bin_splitter_t>::model_builder;
    };
    struct model_episode_t : model_episode<model_builder_t, model_sizer_t, best_model_tracker> {
        using model_episode<model_builder_t, model_sizer_t, best_model_tracker>::model_episode;
    };
    struct feature_evaluator_t : feature_evaluator<primitive_caller_t> {
        using feature_evaluator<primitive_caller_t>::feature_evaluator;
    };
    struct model_evaluator_t : model_evaluator<feature_evaluator_t> {
        using model_evaluator<feature_evaluator_t>::model_evaluator;
    };

    scout_manifest(
        schema& types,
        value_pool& values,
        const std::vector<typed_primitive>& primitives,
        const scout_options& options);

    schema&                     schema_;
    value_pool&                 values_;
    constant_store              constants_;
    cheapest_feature_index      feature_candidates_;
    application_log             applications_;
    output_finder               output_finder_;
    uniform_value_finder        uniform_value_finder_;
    value_sizer                 value_sizer_;
    feature_sizer_t             feature_sizer_;
    model_sizer_t               model_sizer_;
    primitive_store             primitive_store_;
    primitive_caller_t          primitive_caller_;
    variable_candidate_finder_t variable_candidate_finder_;
    constant_seeder_t           constant_seeder_;
    operand_table_t             operand_table_;
    operand_applier_t           operand_applier_;
    app_candidate_finder_t      app_candidate_finder_;
    witness_detector_t          witness_detector_;
    primitive_option_finder_t   primitive_option_finder_;
    argument_option_finder_t    argument_option_finder_;
    first_option_chooser        option_chooser_;
    argument_chooser_t          argument_chooser_;
    primitive_applier_t         primitive_applier_;
    synthesis_starter_t         synthesis_starter_;
    feature_synthesizer_t       feature_synthesizer_;
    leaf_detector_t             leaf_detector_;
    bin_splitter_t              bin_splitter_;
    bound_argument_chooser_t    bound_argument_chooser_;
    model_builder_t             model_builder_;
    best_model_tracker          best_model_tracker_;
    model_episode_t             model_episode_;
    feature_evaluator_t         feature_evaluator_;
    model_evaluator_t           model_evaluator_;
};

#endif
