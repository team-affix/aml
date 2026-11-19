#ifndef RAZOR_MANIFEST_HPP
#define RAZOR_MANIFEST_HPP

#include <random>
#include <unordered_map>
#include <vector>
#include "infrastructure/app_candidate_finder.hpp"
#include "infrastructure/application_log.hpp"
#include "infrastructure/argument_chooser.hpp"
#include "infrastructure/argument_option_finder.hpp"
#include "infrastructure/best_model_tracker.hpp"
#include "infrastructure/bin_splitter.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/constant_enumerator.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_pool.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/feature_synthesizer.hpp"
#include "infrastructure/field_chooser.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/leaf_detector.hpp"
#include "infrastructure/mcts_episode.hpp"
#include "infrastructure/mcts_option_chooser.hpp"
#include "infrastructure/model_builder.hpp"
#include "infrastructure/model_episode.hpp"
#include "infrastructure/model_evaluator.hpp"
#include "infrastructure/model_node_pool.hpp"
#include "infrastructure/model_sizer.hpp"
#include "infrastructure/operand_applier.hpp"
#include "infrastructure/operand_table.hpp"
#include "infrastructure/option_range.hpp"
#include "infrastructure/output_finder.hpp"
#include "infrastructure/primitive.hpp"
#include "infrastructure/primitive_applier.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_option_finder.hpp"
#include "infrastructure/primitive_signature_reader.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/random_rollout.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/synthesis_starter.hpp"
#include "infrastructure/tree_walker.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/uct_backprop_path.hpp"
#include "infrastructure/uct_chooser.hpp"
#include "infrastructure/uct_cursor.hpp"
#include "infrastructure/uct_terminator.hpp"
#include "infrastructure/uct_value_creditor.hpp"
#include "infrastructure/uct_visit_creditor.hpp"
#include "infrastructure/uniform_exploration_constant.hpp"
#include "infrastructure/uniform_value_delta.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/uniform_value_update.hpp"
#include "infrastructure/value_pool.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "infrastructure/visits_table.hpp"
#include "infrastructure/witness_detector.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"
#include "value_objects/razor_options.hpp"

// Razor: Monte Carlo tree search over every choice a model build makes (the feature each partition matches on and
// the field each funnel step descends into), rewarding each episode with its negated model cost.
struct razor_manifest {
    struct visits_table_t : monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map> {
        using monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>::visits_table;
    };
    struct value_table_t : monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map> {
        using monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>::value_table;
    };
    struct rollout_t : monte_carlo::random_rollout<mcts_choice, std::mt19937, option_range, option_range> {
        using monte_carlo::random_rollout<mcts_choice, std::mt19937, option_range, option_range>::random_rollout;
    };
    struct exploration_constant_t : monte_carlo::uniform_exploration_constant<double> {
        using monte_carlo::uniform_exploration_constant<double>::uniform_exploration_constant;
    };
    struct value_delta_t : monte_carlo::uniform_value_delta<double> {
        using monte_carlo::uniform_value_delta<double>::uniform_value_delta;
    };
    struct value_update_t
        : monte_carlo::uniform_value_update<mcts_tree_node_id, value_table_t, value_table_t, value_delta_t> {
        using monte_carlo::uniform_value_update<
            mcts_tree_node_id, value_table_t, value_table_t, value_delta_t>::uniform_value_update;
    };
    struct cursor_t : monte_carlo::uct_cursor<mcts_tree_node_id> {
        using monte_carlo::uct_cursor<mcts_tree_node_id>::uct_cursor;
    };
    struct backprop_path_t : monte_carlo::uct_backprop_path<mcts_tree_node_id> {
        using monte_carlo::uct_backprop_path<mcts_tree_node_id>::uct_backprop_path;
    };
    struct visit_creditor_t
        : monte_carlo::uct_visit_creditor<mcts_tree_node_id, backprop_path_t, visits_table_t, visits_table_t> {
        using monte_carlo::uct_visit_creditor<
            mcts_tree_node_id, backprop_path_t, visits_table_t, visits_table_t>::uct_visit_creditor;
    };
    struct value_creditor_t : monte_carlo::uct_value_creditor<visit_creditor_t, backprop_path_t, value_update_t> {
        using monte_carlo::uct_value_creditor<visit_creditor_t, backprop_path_t, value_update_t>::uct_value_creditor;
    };
    struct policy_t : monte_carlo::ucb1<
        mcts_tree_node_id, mcts_choice, double, visits_table_t, value_table_t, tree_walker, exploration_constant_t,
        option_range, option_range> {
        using monte_carlo::ucb1<
            mcts_tree_node_id, mcts_choice, double, visits_table_t, value_table_t, tree_walker,
            exploration_constant_t, option_range, option_range>::ucb1;
    };
    struct mcts_chooser_t : monte_carlo::uct_chooser<
        mcts_tree_node_id, mcts_choice, visits_table_t, tree_walker, option_range, option_range, policy_t, rollout_t,
        cursor_t, cursor_t, backprop_path_t, monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag> {
        using monte_carlo::uct_chooser<
            mcts_tree_node_id, mcts_choice, visits_table_t, tree_walker, option_range, option_range, policy_t,
            rollout_t, cursor_t, cursor_t, backprop_path_t, monte_carlo::in_rollout_flag,
            monte_carlo::in_rollout_flag>::uct_chooser;
    };
    struct terminator_t : monte_carlo::uct_terminator<
        mcts_tree_node_id, backprop_path_t, value_creditor_t, backprop_path_t, backprop_path_t, cursor_t,
        monte_carlo::in_rollout_flag> {
        using monte_carlo::uct_terminator<
            mcts_tree_node_id, backprop_path_t, value_creditor_t, backprop_path_t, backprop_path_t, cursor_t,
            monte_carlo::in_rollout_flag>::uct_terminator;
    };

    struct option_chooser_t : mcts_option_chooser<mcts_chooser_t> {
        using mcts_option_chooser<mcts_chooser_t>::mcts_option_chooser;
    };

    struct feature_sizer_t : feature_sizer<value_sizer> {
        using feature_sizer<value_sizer>::feature_sizer;
    };
    struct model_sizer_t : model_sizer<feature_sizer_t> {
        using model_sizer<feature_sizer_t>::model_sizer;
    };
    struct primitive_store_t : primitive_store<primitive> {
        using primitive_store<primitive>::primitive_store;
    };
    struct signature_reader_t : primitive_signature_reader<primitive_store_t> {
        using primitive_signature_reader<primitive_store_t>::primitive_signature_reader;
    };
    struct primitive_caller_t : primitive_caller<primitive_store_t> {
        using primitive_caller<primitive_store_t>::primitive_caller;
    };
    struct variable_candidate_finder_t
        : variable_candidate_finder<feature_pool, feature_sizer_t, uniform_value_finder> {
        using variable_candidate_finder<feature_pool, feature_sizer_t, uniform_value_finder>::variable_candidate_finder;
    };
    struct constant_enumerator_t : constant_enumerator<schema, schema, value_pool> {
        using constant_enumerator<schema, schema, value_pool>::constant_enumerator;
    };
    struct constant_seeder_t : constant_seeder<schema, constant_enumerator_t, feature_pool, constant_store> {
        using constant_seeder<schema, constant_enumerator_t, feature_pool, constant_store>::constant_seeder;
    };
    struct operand_table_t
        : operand_table<constant_store, constant_store, cheapest_feature_index, cheapest_feature_index> {
        using operand_table<
            constant_store, constant_store, cheapest_feature_index, cheapest_feature_index>::operand_table;
    };
    struct operand_applier_t : operand_applier<operand_table_t, primitive_caller_t> {
        using operand_applier<operand_table_t, primitive_caller_t>::operand_applier;
    };
    struct app_candidate_finder_t : app_candidate_finder<
        operand_applier_t, uniform_value_finder, operand_table_t, feature_pool, feature_sizer_t> {
        using app_candidate_finder<
            operand_applier_t, uniform_value_finder, operand_table_t, feature_pool,
            feature_sizer_t>::app_candidate_finder;
    };
    struct witness_detector_t
        : witness_detector<operand_table_t, operand_table_t, operand_table_t, application_log> {
        using witness_detector<operand_table_t, operand_table_t, operand_table_t, application_log>::witness_detector;
    };
    struct primitive_option_finder_t
        : primitive_option_finder<primitive_store_t, signature_reader_t, witness_detector_t> {
        using primitive_option_finder<
            primitive_store_t, signature_reader_t, witness_detector_t>::primitive_option_finder;
    };
    struct argument_option_finder_t
        : argument_option_finder<operand_table_t, operand_table_t, witness_detector_t> {
        using argument_option_finder<operand_table_t, operand_table_t, witness_detector_t>::argument_option_finder;
    };
    struct argument_chooser_t : argument_chooser<argument_option_finder_t, option_chooser_t> {
        using argument_chooser<argument_option_finder_t, option_chooser_t>::argument_chooser;
    };
    struct primitive_applier_t : primitive_applier<
        signature_reader_t, argument_chooser_t, application_log, app_candidate_finder_t, cheapest_feature_index> {
        using primitive_applier<
            signature_reader_t, argument_chooser_t, application_log, app_candidate_finder_t,
            cheapest_feature_index>::primitive_applier;
    };
    struct synthesis_starter_t : synthesis_starter<
        cheapest_feature_index, application_log, variable_candidate_finder_t, cheapest_feature_index> {
        using synthesis_starter<
            cheapest_feature_index, application_log, variable_candidate_finder_t,
            cheapest_feature_index>::synthesis_starter;
    };
    struct feature_synthesizer_t : feature_synthesizer<
        synthesis_starter_t, application_log, primitive_option_finder_t, option_chooser_t, primitive_applier_t,
        cheapest_feature_index, output_finder> {
        using feature_synthesizer<
            synthesis_starter_t, application_log, primitive_option_finder_t, option_chooser_t, primitive_applier_t,
            cheapest_feature_index, output_finder>::feature_synthesizer;
    };
    struct leaf_detector_t : leaf_detector<feature_pool, uniform_value_finder, feature_pool> {
        using leaf_detector<feature_pool, uniform_value_finder, feature_pool>::leaf_detector;
    };
    struct bin_splitter_t : bin_splitter<schema, schema> {
        using bin_splitter<schema, schema>::bin_splitter;
    };
    struct field_chooser_t : field_chooser<variable_candidate_finder_t, option_chooser_t> {
        using field_chooser<variable_candidate_finder_t, option_chooser_t>::field_chooser;
    };
    struct model_builder_t : model_builder<
        leaf_detector_t, feature_synthesizer_t, output_finder, option_chooser_t, field_chooser_t, bin_splitter_t,
        model_node_pool, model_node_pool, model_node_pool> {
        using model_builder<
            leaf_detector_t, feature_synthesizer_t, output_finder, option_chooser_t, field_chooser_t, bin_splitter_t,
            model_node_pool, model_node_pool, model_node_pool>::model_builder;
    };
    struct model_episode_t : model_episode<model_builder_t, model_sizer_t, best_model_tracker> {
        using model_episode<model_builder_t, model_sizer_t, best_model_tracker>::model_episode;
    };
    struct mcts_episode_t : mcts_episode<model_episode_t, value_delta_t, terminator_t> {
        using mcts_episode<model_episode_t, value_delta_t, terminator_t>::mcts_episode;
    };
    struct feature_evaluator_t : feature_evaluator<primitive_caller_t> {
        using feature_evaluator<primitive_caller_t>::feature_evaluator;
    };
    struct model_evaluator_t : model_evaluator<feature_evaluator_t, schema> {
        using model_evaluator<feature_evaluator_t, schema>::model_evaluator;
    };

    razor_manifest(
        schema& types, value_pool& values, const std::vector<primitive>& primitives, const razor_options& options);

    schema&                      schema_;
    value_pool&                  values_;
    feature_pool                 features_;
    model_node_pool              nodes_;
    constant_store               constants_;
    cheapest_feature_index       feature_candidates_;
    application_log              applications_;
    output_finder                output_finder_;
    uniform_value_finder         uniform_value_finder_;
    value_sizer                  value_sizer_;
    feature_sizer_t              feature_sizer_;
    model_sizer_t                model_sizer_;
    primitive_store_t            primitive_store_;
    signature_reader_t           signature_reader_;
    primitive_caller_t           primitive_caller_;
    variable_candidate_finder_t  variable_candidate_finder_;
    constant_enumerator_t        constant_enumerator_;
    constant_seeder_t            constant_seeder_;
    operand_table_t              operand_table_;
    operand_applier_t            operand_applier_;
    app_candidate_finder_t       app_candidate_finder_;
    witness_detector_t           witness_detector_;
    primitive_option_finder_t    primitive_option_finder_;
    argument_option_finder_t     argument_option_finder_;
    visits_table_t               visits_;
    value_table_t                rewards_;
    std::mt19937                 random_;
    tree_walker                  tree_walker_;
    rollout_t                    rollout_;
    exploration_constant_t       exploration_constant_;
    value_delta_t                value_delta_;
    value_update_t               value_update_;
    cursor_t                     cursor_;
    backprop_path_t              backprop_path_;
    visit_creditor_t             visit_creditor_;
    value_creditor_t             value_creditor_;
    policy_t                     policy_;
    monte_carlo::in_rollout_flag in_rollout_;
    mcts_chooser_t               mcts_chooser_;
    terminator_t                 terminator_;
    option_chooser_t             option_chooser_;
    argument_chooser_t           argument_chooser_;
    primitive_applier_t          primitive_applier_;
    synthesis_starter_t          synthesis_starter_;
    feature_synthesizer_t        feature_synthesizer_;
    leaf_detector_t              leaf_detector_;
    bin_splitter_t               bin_splitter_;
    field_chooser_t              field_chooser_;
    model_builder_t              model_builder_;
    best_model_tracker           best_model_tracker_;
    model_episode_t              model_episode_;
    mcts_episode_t               mcts_episode_;
    feature_evaluator_t          feature_evaluator_;
    model_evaluator_t            model_evaluator_;
};

#endif
