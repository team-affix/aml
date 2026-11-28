#ifndef OBSIDIAN_MANIFEST_HPP
#define OBSIDIAN_MANIFEST_HPP

#include <cstddef>
#include <random>
#include <unordered_map>
#include <vector>
#include "infrastructure/ancestry_finder.hpp"
#include "infrastructure/ancestry_sizer.hpp"
#include "infrastructure/application_chooser.hpp"
#include "infrastructure/argument_type_index.hpp"
#include "infrastructure/best_table_tracker.hpp"
#include "infrastructure/cached_column_maker.hpp"
#include "infrastructure/column_byte_measurer.hpp"
#include "infrastructure/column_cache.hpp"
#include "infrastructure/column_computer.hpp"
#include "infrastructure/column_hash_index.hpp"
#include "infrastructure/column_reader.hpp"
#include "infrastructure/constant_ancestry_finder.hpp"
#include "infrastructure/constant_atom_seeder.hpp"
#include "infrastructure/constant_byte_measurer.hpp"
#include "infrastructure/constant_cache.hpp"
#include "infrastructure/constant_computer.hpp"
#include "infrastructure/constant_graph.hpp"
#include "infrastructure/constant_reader.hpp"
#include "infrastructure/derivation_index.hpp"
#include "infrastructure/distinct_row_counter.hpp"
#include "infrastructure/droppable_member_finder.hpp"
#include "infrastructure/eligible_primitive_finder.hpp"
#include "infrastructure/equal_feature_finder.hpp"
#include "infrastructure/feature_deriver.hpp"
#include "infrastructure/feature_graph.hpp"
#include "infrastructure/feature_recorder.hpp"
#include "infrastructure/fillable_primitive_finder.hpp"
#include "infrastructure/graph_row_evaluator.hpp"
#include "infrastructure/in_rollout_flag.hpp"
#include "infrastructure/input_feature_seeder.hpp"
#include "infrastructure/key_drawer.hpp"
#include "infrastructure/mcts_option_chooser.hpp"
#include "infrastructure/mcts_walk_episode.hpp"
#include "infrastructure/max_value_update.hpp"
#include "infrastructure/mixmax_value_view.hpp"
#include "infrastructure/model_graph_builder.hpp"
#include "infrastructure/native_primitive_store.hpp"
#include "infrastructure/native_type_store.hpp"
#include "infrastructure/operand_pool_finder.hpp"
#include "infrastructure/option_range.hpp"
#include "infrastructure/origin_replacer.hpp"
#include "infrastructure/random_rollout.hpp"
#include "infrastructure/row_key_numberer.hpp"
#include "infrastructure/set_cost_measurer.hpp"
#include "infrastructure/set_mover.hpp"
#include "infrastructure/set_walk_episode.hpp"
#include "infrastructure/table_model_builder.hpp"
#include "infrastructure/table_model_evaluator.hpp"
#include "infrastructure/tree_walker.hpp"
#include "infrastructure/ucb1.hpp"
#include "infrastructure/uct_backprop_path.hpp"
#include "infrastructure/uct_chooser.hpp"
#include "infrastructure/uct_cursor.hpp"
#include "infrastructure/uct_terminator.hpp"
#include "infrastructure/uct_value_creditor.hpp"
#include "infrastructure/uct_visit_creditor.hpp"
#include "infrastructure/uniform_column_detector.hpp"
#include "infrastructure/uniform_exploration_constant.hpp"
#include "infrastructure/uniform_value_delta.hpp"
#include "infrastructure/value_bounds.hpp"
#include "infrastructure/value_class_finder.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/visits_table.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"
#include "value_objects/native_bin.hpp"
#include "value_objects/native_constant.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/native_type.hpp"
#include "value_objects/obsidian_options.hpp"

namespace AML {

// Obsidian: Quartz's sets, features, constants and costs searched by UCT in place of best-first. Each episode walks
// from the root set, the inputs, by up to max_moves moves, each one drop or one addition chosen through flat MCTS
// choices (drop or add, then the member; or the primitive, then each argument), and is rewarded with the negated cost
// of the cheapest set it passed through. A node keeps the best reward backed up through it, not the mean, and
// selection reads it as that best (mixmax with the best weighted fully). Features persist in the graph across
// episodes, so a cheaper origin found by any episode prices every later set.
struct obsidian_manifest {
    struct visits_table_t : monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map> {
        using monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>::visits_table;
    };
    struct value_table_t : monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map> {
        using monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>::value_table;
    };
    struct rollout_t : monte_carlo::random_rollout<mcts_choice, std::mt19937_64, option_range, option_range> {
        using monte_carlo::random_rollout<mcts_choice, std::mt19937_64, option_range, option_range>::random_rollout;
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
    struct value_update_t : monte_carlo::max_value_update<
        mcts_tree_node_id, visits_table_t, value_table_t, value_table_t, value_delta_t> {
        using monte_carlo::max_value_update<
            mcts_tree_node_id, visits_table_t, value_table_t, value_table_t, value_delta_t>::max_value_update;
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
    struct column_computer_t : column_computer<native_primitive_store> {
        using column_computer<native_primitive_store>::column_computer;
    };
    struct cached_column_maker_t : cached_column_maker<native_type_store> {
        using cached_column_maker<native_type_store>::cached_column_maker;
    };
    struct column_byte_measurer_t : column_byte_measurer<native_type_store> {
        using column_byte_measurer<native_type_store>::column_byte_measurer;
    };
    struct constant_computer_t : constant_computer<native_primitive_store, native_type_store> {
        using constant_computer<native_primitive_store, native_type_store>::constant_computer;
    };
    struct constant_byte_measurer_t : constant_byte_measurer<native_type_store> {
        using constant_byte_measurer<native_type_store>::constant_byte_measurer;
    };
    struct constant_reader_t : constant_reader<
        constant_graph, constant_cache, constant_computer_t, constant_byte_measurer_t, constant_cache> {
        using constant_reader<
            constant_graph, constant_cache, constant_computer_t, constant_byte_measurer_t,
            constant_cache>::constant_reader;
    };
    struct column_reader_t : column_reader<
        feature_graph, constant_reader_t, column_cache, column_computer_t, cached_column_maker_t,
        column_byte_measurer_t, column_cache> {
        using column_reader<
            feature_graph, constant_reader_t, column_cache, column_computer_t, cached_column_maker_t,
            column_byte_measurer_t, column_cache>::column_reader;
    };
    struct ancestry_finder_t : ancestry_finder<feature_graph> {
        using ancestry_finder<feature_graph>::ancestry_finder;
    };
    struct constant_ancestry_finder_t : constant_ancestry_finder<constant_graph> {
        using constant_ancestry_finder<constant_graph>::constant_ancestry_finder;
    };
    struct ancestry_sizer_t : ancestry_sizer<ancestry_finder_t, feature_graph, constant_ancestry_finder_t> {
        using ancestry_sizer<ancestry_finder_t, feature_graph, constant_ancestry_finder_t>::ancestry_sizer;
    };
    struct distinct_row_counter_t : distinct_row_counter<column_reader_t, row_key_numberer> {
        using distinct_row_counter<column_reader_t, row_key_numberer>::distinct_row_counter;
    };
    struct droppable_member_finder_t : droppable_member_finder<column_reader_t, row_key_numberer> {
        using droppable_member_finder<column_reader_t, row_key_numberer>::droppable_member_finder;
    };
    struct key_drawer_t : key_drawer<std::mt19937_64> {
        using key_drawer<std::mt19937_64>::key_drawer;
    };
    struct equal_feature_finder_t : equal_feature_finder<column_hash_index, column_reader_t, native_type_store> {
        using equal_feature_finder<column_hash_index, column_reader_t, native_type_store>::equal_feature_finder;
    };
    struct origin_replacer_t : origin_replacer<ancestry_sizer_t, feature_graph> {
        using origin_replacer<ancestry_sizer_t, feature_graph>::origin_replacer;
    };
    struct feature_recorder_t : feature_recorder<
        key_drawer_t, feature_graph, derivation_index, column_hash_index, column_byte_measurer_t, column_cache> {
        using feature_recorder<
            key_drawer_t, feature_graph, derivation_index, column_hash_index, column_byte_measurer_t,
            column_cache>::feature_recorder;
    };
    struct feature_deriver_t : feature_deriver<
        derivation_index, column_reader_t, constant_reader_t, column_computer_t, native_primitive_store,
        cached_column_maker_t, uniform_column_detector, equal_feature_finder_t, origin_replacer_t, derivation_index,
        feature_recorder_t> {
        using feature_deriver<
            derivation_index, column_reader_t, constant_reader_t, column_computer_t, native_primitive_store,
            cached_column_maker_t, uniform_column_detector, equal_feature_finder_t, origin_replacer_t,
            derivation_index, feature_recorder_t>::feature_deriver;
    };
    struct operand_pool_finder_t : operand_pool_finder<feature_graph> {
        using operand_pool_finder<feature_graph>::operand_pool_finder;
    };
    struct eligible_primitive_finder_t : eligible_primitive_finder<argument_type_index> {
        using eligible_primitive_finder<argument_type_index>::eligible_primitive_finder;
    };
    struct fillable_primitive_finder_t : fillable_primitive_finder<native_primitive_store, constant_graph> {
        using fillable_primitive_finder<native_primitive_store, constant_graph>::fillable_primitive_finder;
    };
    struct application_chooser_t : application_chooser<native_primitive_store, constant_graph, option_chooser_t> {
        using application_chooser<native_primitive_store, constant_graph, option_chooser_t>::application_chooser;
    };
    struct set_mover_t : set_mover<
        droppable_member_finder_t, ancestry_finder_t, operand_pool_finder_t, eligible_primitive_finder_t,
        fillable_primitive_finder_t, option_chooser_t, application_chooser_t, feature_deriver_t> {
        using set_mover<
            droppable_member_finder_t, ancestry_finder_t, operand_pool_finder_t, eligible_primitive_finder_t,
            fillable_primitive_finder_t, option_chooser_t, application_chooser_t, feature_deriver_t>::set_mover;
    };
    struct set_cost_measurer_t : set_cost_measurer<ancestry_sizer_t, distinct_row_counter_t> {
        using set_cost_measurer<ancestry_sizer_t, distinct_row_counter_t>::set_cost_measurer;
    };
    struct model_graph_builder_t : model_graph_builder<feature_graph> {
        using model_graph_builder<feature_graph>::model_graph_builder;
    };
    struct table_model_builder_t : table_model_builder<model_graph_builder_t, column_reader_t, native_type_store> {
        using table_model_builder<model_graph_builder_t, column_reader_t, native_type_store>::table_model_builder;
    };
    struct set_walk_episode_t
        : set_walk_episode<set_mover_t, set_cost_measurer_t, table_model_builder_t, best_table_tracker> {
        using set_walk_episode<
            set_mover_t, set_cost_measurer_t, table_model_builder_t, best_table_tracker>::set_walk_episode;
    };
    struct mcts_walk_episode_t : mcts_walk_episode<set_walk_episode_t, value_delta_t, terminator_t> {
        using mcts_walk_episode<set_walk_episode_t, value_delta_t, terminator_t>::mcts_walk_episode;
    };
    struct input_feature_seeder_t : input_feature_seeder<
        column_computer_t, cached_column_maker_t, feature_recorder_t, uniform_column_detector> {
        using input_feature_seeder<
            column_computer_t, cached_column_maker_t, feature_recorder_t,
            uniform_column_detector>::input_feature_seeder;
    };
    struct constant_atom_seeder_t : constant_atom_seeder<constant_graph> {
        using constant_atom_seeder<constant_graph>::constant_atom_seeder;
    };
    struct graph_row_evaluator_t : graph_row_evaluator<native_primitive_store, native_type_store, constant_reader_t> {
        using graph_row_evaluator<native_primitive_store, native_type_store, constant_reader_t>::graph_row_evaluator;
    };
    struct value_class_finder_t : value_class_finder<native_type_store> {
        using value_class_finder<native_type_store>::value_class_finder;
    };
    struct table_model_evaluator_t : table_model_evaluator<graph_row_evaluator_t, value_class_finder_t> {
        using table_model_evaluator<graph_row_evaluator_t, value_class_finder_t>::table_model_evaluator;
    };

    obsidian_manifest(
        const native_bin& training,
        const std::vector<native_constant>& constants,
        const std::vector<native_type>& types,
        const std::vector<native_primitive>& primitives,
        const std::vector<feature_id>& root,
        const obsidian_options& options);

    std::mt19937_64              random_;
    row_key_numberer             row_key_numberer_;
    uniform_column_detector      uniform_column_detector_;
    native_type_store            native_types_;
    native_primitive_store       native_primitives_;
    argument_type_index          argument_types_;
    std::vector<size_t>          label_classes_;
    feature_graph                feature_graph_;
    constant_graph               constant_graph_;
    derivation_index             derivation_index_;
    column_hash_index            column_hash_index_;
    column_cache                 column_cache_;
    constant_cache               constant_cache_;
    best_table_tracker           best_table_tracker_;
    visits_table_t               visits_;
    value_table_t                best_rewards_;
    value_bounds_t               reward_bounds_;
    monte_carlo::in_rollout_flag in_rollout_;
    tree_walker_t                tree_walker_;
    rollout_t                    rollout_;
    exploration_constant_t       exploration_constant_;
    value_delta_t                value_delta_;
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
    column_computer_t            column_computer_;
    cached_column_maker_t        cached_column_maker_;
    column_byte_measurer_t       column_byte_measurer_;
    constant_computer_t          constant_computer_;
    constant_byte_measurer_t     constant_byte_measurer_;
    constant_reader_t            constant_reader_;
    column_reader_t              column_reader_;
    ancestry_finder_t            ancestry_finder_;
    constant_ancestry_finder_t   constant_ancestry_finder_;
    ancestry_sizer_t             ancestry_sizer_;
    distinct_row_counter_t       distinct_row_counter_;
    droppable_member_finder_t    droppable_member_finder_;
    key_drawer_t                 key_drawer_;
    equal_feature_finder_t       equal_feature_finder_;
    origin_replacer_t            origin_replacer_;
    feature_recorder_t           feature_recorder_;
    feature_deriver_t            feature_deriver_;
    operand_pool_finder_t        operand_pool_finder_;
    eligible_primitive_finder_t  eligible_primitive_finder_;
    fillable_primitive_finder_t  fillable_primitive_finder_;
    application_chooser_t        application_chooser_;
    set_mover_t                  set_mover_;
    set_cost_measurer_t          set_cost_measurer_;
    model_graph_builder_t        model_graph_builder_;
    table_model_builder_t        table_model_builder_;
    set_walk_episode_t           set_walk_episode_;
    mcts_walk_episode_t          mcts_walk_episode_;
    input_feature_seeder_t       input_feature_seeder_;
    constant_atom_seeder_t       constant_atom_seeder_;
    graph_row_evaluator_t        graph_row_evaluator_;
    value_class_finder_t         value_class_finder_;
    table_model_evaluator_t      table_model_evaluator_;
};

}

#endif
