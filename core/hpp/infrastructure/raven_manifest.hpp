#ifndef RAVEN_MANIFEST_HPP
#define RAVEN_MANIFEST_HPP

#include <random>
#include <unordered_map>
#include <vector>
#include "infrastructure/app_candidate_finder.hpp"
#include "infrastructure/application_log.hpp"
#include "infrastructure/argument_option_finder.hpp"
#include "infrastructure/best_model_tracker.hpp"
#include "infrastructure/binary_option_chooser.hpp"
#include "infrastructure/bin_splitter.hpp"
#include "infrastructure/bound_argument_chooser.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_column_evaluator.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_quadrant_measurer.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/feature_synthesizer.hpp"
#include "infrastructure/fraction_drawer.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/inheriting_bin_splitter.hpp"
#include "infrastructure/inheriting_model_evaluator.hpp"
#include "infrastructure/keyed_argument_chooser.hpp"
#include "infrastructure/keyed_feature_chooser.hpp"
#include "infrastructure/leaf_detector.hpp"
#include "infrastructure/max_value_update.hpp"
#include "infrastructure/mcts_episode.hpp"
#include "infrastructure/mcts_option_chooser.hpp"
#include "infrastructure/mixmax_value_view.hpp"
#include "infrastructure/model_builder.hpp"
#include "infrastructure/model_episode.hpp"
#include "infrastructure/model_sizer.hpp"
#include "infrastructure/operand_applier.hpp"
#include "infrastructure/operand_quadrant_measurer.hpp"
#include "infrastructure/operand_table.hpp"
#include "infrastructure/option_range.hpp"
#include "infrastructure/output_finder.hpp"
#include "infrastructure/paired_value_update.hpp"
#include "infrastructure/primitive_applier.hpp"
#include "infrastructure/primitive_caller.hpp"
#include "infrastructure/primitive_option_finder.hpp"
#include "infrastructure/primitive_store.hpp"
#include "infrastructure/quadrant_option_chooser.hpp"
#include "infrastructure/random_rollout.hpp"
#include "infrastructure/rollout_switching_option_chooser.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/scrutinee_piece_finder.hpp"
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
#include "infrastructure/value_bounds.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "infrastructure/visits_table.hpp"
#include "infrastructure/weighted_quadrant_chooser.hpp"
#include "infrastructure/witness_detector.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"
#include "value_objects/raven_options.hpp"
#include "value_objects/typed_primitive.hpp"

namespace AML {

// Raven: Wren whose matches pass what they built down as variables. Below a match, a child's variables are its
// parent's, its constructor's arguments, then the scrutinee and every primitive application inside it, each once
// (scrutinee_piece_finder), evaluated once over the parent's rows and selected for each child (inheriting_bin_splitter,
// feature_column_evaluator). A piece is paid for once, inside the match's scrutinee, and costs 1 wherever it is used
// below, like any variable; pieces from further up arrive as the parent's variables. A piece uniform over a bin stays
// among its variables, so the numbering does not depend on the rows, and is never offered to its sessions. Inference
// binds the same variables (inheriting_model_evaluator). Everything else is Wren's.
struct raven_manifest {
    struct visits_table_t : monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map> {
        using monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>::visits_table;
    };
    struct value_table_t : monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map> {
        using monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>::value_table;
    };
    struct rollout_t : monte_carlo::random_rollout<mcts_choice, std::mt19937, option_range, option_range> {
        using monte_carlo::random_rollout<mcts_choice, std::mt19937, option_range, option_range>::random_rollout;
    };
    struct value_bounds_t : monte_carlo::value_bounds<double> {
        using monte_carlo::value_bounds<double>::value_bounds;
    };
    struct exploration_constant_t : monte_carlo::uniform_exploration_constant<double> {
        using monte_carlo::uniform_exploration_constant<double>::uniform_exploration_constant;
    };
    struct value_delta_t : monte_carlo::uniform_value_delta<double> {
        using monte_carlo::uniform_value_delta<double>::uniform_value_delta;
    };
    struct sum_update_t
        : monte_carlo::uniform_value_update<mcts_tree_node_id, value_table_t, value_table_t, value_delta_t> {
        using monte_carlo::uniform_value_update<
            mcts_tree_node_id, value_table_t, value_table_t, value_delta_t>::uniform_value_update;
    };
    struct best_update_t : monte_carlo::max_value_update<
        mcts_tree_node_id, visits_table_t, value_table_t, value_table_t, value_delta_t> {
        using monte_carlo::max_value_update<
            mcts_tree_node_id, visits_table_t, value_table_t, value_table_t, value_delta_t>::max_value_update;
    };
    struct value_update_t : monte_carlo::paired_value_update<mcts_tree_node_id, sum_update_t, best_update_t> {
        using monte_carlo::paired_value_update<mcts_tree_node_id, sum_update_t, best_update_t>::paired_value_update;
    };
    struct value_view_t : monte_carlo::mixmax_value_view<
        mcts_tree_node_id, double, visits_table_t, value_table_t, value_table_t> {
        using monte_carlo::mixmax_value_view<
            mcts_tree_node_id, double, visits_table_t, value_table_t, value_table_t>::mixmax_value_view;
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
    struct tree_walker_t : tree_walker<monte_carlo::in_rollout_flag> {
        using tree_walker<monte_carlo::in_rollout_flag>::tree_walker;
    };
    struct policy_t : monte_carlo::ucb1<
        mcts_tree_node_id, mcts_choice, double, visits_table_t, value_view_t, value_bounds_t, value_bounds_t,
        tree_walker_t, exploration_constant_t, option_range, option_range> {
        using monte_carlo::ucb1<
            mcts_tree_node_id, mcts_choice, double, visits_table_t, value_view_t, value_bounds_t, value_bounds_t,
            tree_walker_t, exploration_constant_t, option_range, option_range>::ucb1;
    };
    struct mcts_chooser_t : monte_carlo::uct_chooser<
        mcts_tree_node_id, mcts_choice, visits_table_t, tree_walker_t, option_range, option_range, policy_t, rollout_t,
        cursor_t, cursor_t, backprop_path_t, monte_carlo::in_rollout_flag, monte_carlo::in_rollout_flag> {
        using monte_carlo::uct_chooser<
            mcts_tree_node_id, mcts_choice, visits_table_t, tree_walker_t, option_range, option_range, policy_t,
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
    struct binary_option_chooser_t : binary_option_chooser<option_chooser_t> {
        using binary_option_chooser<option_chooser_t>::binary_option_chooser;
    };

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
    struct operand_quadrant_measurer_t : operand_quadrant_measurer<operand_table_t, application_log> {
        using operand_quadrant_measurer<operand_table_t, application_log>::operand_quadrant_measurer;
    };
    struct feature_quadrant_measurer_t : feature_quadrant_measurer<operand_table_t, operand_quadrant_measurer_t> {
        using feature_quadrant_measurer<operand_table_t, operand_quadrant_measurer_t>::feature_quadrant_measurer;
    };
    struct quadrant_option_chooser_t : quadrant_option_chooser<option_chooser_t, binary_option_chooser_t> {
        using quadrant_option_chooser<option_chooser_t, binary_option_chooser_t>::quadrant_option_chooser;
    };
    struct fraction_drawer_t : fraction_drawer<std::mt19937> {
        using fraction_drawer<std::mt19937>::fraction_drawer;
    };
    struct weighted_quadrant_chooser_t : weighted_quadrant_chooser<fraction_drawer_t, option_chooser_t> {
        using weighted_quadrant_chooser<fraction_drawer_t, option_chooser_t>::weighted_quadrant_chooser;
    };
    struct operand_chooser_t : rollout_switching_option_chooser<
        monte_carlo::in_rollout_flag, quadrant_option_chooser_t, weighted_quadrant_chooser_t> {
        using rollout_switching_option_chooser<
            monte_carlo::in_rollout_flag, quadrant_option_chooser_t,
            weighted_quadrant_chooser_t>::rollout_switching_option_chooser;
    };
    struct argument_chooser_t
        : keyed_argument_chooser<argument_option_finder_t, operand_quadrant_measurer_t, operand_chooser_t> {
        using keyed_argument_chooser<
            argument_option_finder_t, operand_quadrant_measurer_t, operand_chooser_t>::keyed_argument_chooser;
    };
    struct feature_chooser_t : keyed_feature_chooser<feature_quadrant_measurer_t, operand_chooser_t> {
        using keyed_feature_chooser<feature_quadrant_measurer_t, operand_chooser_t>::keyed_feature_chooser;
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
    struct feature_column_evaluator_t : feature_column_evaluator<primitive_store, primitive_caller_t> {
        using feature_column_evaluator<primitive_store, primitive_caller_t>::feature_column_evaluator;
    };
    struct feature_synthesizer_t : feature_synthesizer<
        synthesis_starter_t, application_log, primitive_option_finder_t, option_chooser_t,
        binary_option_chooser_t, primitive_applier_t, cheapest_feature_index, output_finder> {
        using feature_synthesizer<
            synthesis_starter_t, application_log, primitive_option_finder_t, option_chooser_t,
            binary_option_chooser_t, primitive_applier_t, cheapest_feature_index, output_finder>::feature_synthesizer;
    };
    struct leaf_detector_t : leaf_detector<uniform_value_finder> {
        using leaf_detector<uniform_value_finder>::leaf_detector;
    };
    struct bin_splitter_t : bin_splitter<schema> {
        using bin_splitter<schema>::bin_splitter;
    };
    struct inheriting_bin_splitter_t : inheriting_bin_splitter<
        bin_splitter_t, cheapest_feature_index, scrutinee_piece_finder, feature_column_evaluator_t> {
        using inheriting_bin_splitter<
            bin_splitter_t, cheapest_feature_index, scrutinee_piece_finder,
            feature_column_evaluator_t>::inheriting_bin_splitter;
    };
    struct bound_argument_chooser_t
        : bound_argument_chooser<variable_candidate_finder_t, binary_option_chooser_t> {
        using bound_argument_chooser<variable_candidate_finder_t, binary_option_chooser_t>::bound_argument_chooser;
    };
    struct model_builder_t : model_builder<
        leaf_detector_t, feature_synthesizer_t, output_finder, feature_chooser_t, bound_argument_chooser_t,
        inheriting_bin_splitter_t> {
        using model_builder<
            leaf_detector_t, feature_synthesizer_t, output_finder, feature_chooser_t, bound_argument_chooser_t,
            inheriting_bin_splitter_t>::model_builder;
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
    struct model_evaluator_t : inheriting_model_evaluator<feature_evaluator_t, scrutinee_piece_finder> {
        using inheriting_model_evaluator<feature_evaluator_t, scrutinee_piece_finder>::inheriting_model_evaluator;
    };

    raven_manifest(
        schema& types,
        const std::vector<typed_primitive>& primitives,
        const raven_options& options);

    schema&                      schema_;
    constant_store               constants_;
    cheapest_feature_index       feature_candidates_;
    application_log              applications_;
    output_finder                output_finder_;
    uniform_value_finder         uniform_value_finder_;
    value_sizer                  value_sizer_;
    feature_sizer_t              feature_sizer_;
    model_sizer_t                model_sizer_;
    primitive_store              primitive_store_;
    primitive_caller_t           primitive_caller_;
    scrutinee_piece_finder       scrutinee_piece_finder_;
    feature_column_evaluator_t   feature_column_evaluator_;
    variable_candidate_finder_t  variable_candidate_finder_;
    constant_seeder_t            constant_seeder_;
    operand_table_t              operand_table_;
    operand_applier_t            operand_applier_;
    app_candidate_finder_t       app_candidate_finder_;
    witness_detector_t           witness_detector_;
    primitive_option_finder_t    primitive_option_finder_;
    argument_option_finder_t     argument_option_finder_;
    visits_table_t               visits_;
    value_table_t                rewards_;
    value_table_t                best_rewards_;
    value_bounds_t               reward_bounds_;
    std::mt19937                 random_;
    monte_carlo::in_rollout_flag in_rollout_;
    tree_walker_t                tree_walker_;
    rollout_t                    rollout_;
    exploration_constant_t       exploration_constant_;
    value_delta_t                value_delta_;
    sum_update_t                 sum_update_;
    best_update_t                best_update_;
    value_update_t               value_update_;
    value_view_t                 value_view_;
    cursor_t                     cursor_;
    backprop_path_t              backprop_path_;
    visit_creditor_t             visit_creditor_;
    value_creditor_t             value_creditor_;
    policy_t                     policy_;
    mcts_chooser_t               mcts_chooser_;
    terminator_t                 terminator_;
    option_chooser_t             option_chooser_;
    binary_option_chooser_t      binary_option_chooser_;
    operand_quadrant_measurer_t  operand_quadrant_measurer_;
    feature_quadrant_measurer_t  feature_quadrant_measurer_;
    quadrant_option_chooser_t    quadrant_option_chooser_;
    fraction_drawer_t            fraction_drawer_;
    weighted_quadrant_chooser_t  weighted_quadrant_chooser_;
    operand_chooser_t            operand_chooser_;
    argument_chooser_t           argument_chooser_;
    feature_chooser_t            feature_chooser_;
    primitive_applier_t          primitive_applier_;
    synthesis_starter_t          synthesis_starter_;
    feature_synthesizer_t        feature_synthesizer_;
    leaf_detector_t              leaf_detector_;
    bin_splitter_t               bin_splitter_;
    inheriting_bin_splitter_t    inheriting_bin_splitter_;
    bound_argument_chooser_t     bound_argument_chooser_;
    model_builder_t              model_builder_;
    best_model_tracker           best_model_tracker_;
    model_episode_t              model_episode_;
    mcts_episode_t               mcts_episode_;
    feature_evaluator_t          feature_evaluator_;
    model_evaluator_t            model_evaluator_;
};

}

#endif
