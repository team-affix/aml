#ifndef RAZOR_MANIFEST_HPP
#define RAZOR_MANIFEST_HPP

#include <random>
#include <unordered_map>
#include <vector>
#include "infrastructure/argument_tuple_enumerator.hpp"
#include "infrastructure/best_model_tracker.hpp"
#include "infrastructure/bin_splitter.hpp"
#include "infrastructure/cheapest_feature_index.hpp"
#include "infrastructure/column_applier.hpp"
#include "infrastructure/constant_enumerator.hpp"
#include "infrastructure/constant_seeder.hpp"
#include "infrastructure/constant_store.hpp"
#include "infrastructure/feature_evaluator.hpp"
#include "infrastructure/feature_pool.hpp"
#include "infrastructure/feature_sizer.hpp"
#include "infrastructure/feature_synthesizer.hpp"
#include "infrastructure/leaf_detector.hpp"
#include "infrastructure/mcts_candidate_chooser.hpp"
#include "infrastructure/mcts_episode.hpp"
#include "infrastructure/model_builder.hpp"
#include "infrastructure/model_episode.hpp"
#include "infrastructure/model_evaluator.hpp"
#include "infrastructure/model_node_pool.hpp"
#include "infrastructure/model_sizer.hpp"
#include "infrastructure/primitive.hpp"
#include "infrastructure/primitive_dispatcher.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/tree_walker.hpp"
#include "infrastructure/uct_value_manifest.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/value_pool.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/value_table.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "infrastructure/visits_table.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"
#include "value_objects/razor_options.hpp"

// Razor: Monte Carlo tree search over every choice a model build makes (the feature each partition matches on and
// the field each funnel step descends into), rewarding each episode with its negated model cost.
struct razor_manifest {
    using choices_t      = std::vector<mcts_choice>;
    using visits_table_t = monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>;
    using value_table_t  = monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>;
    using mcts_t         = monte_carlo::uct_value_manifest<
        mcts_tree_node_id, mcts_choice, double, visits_table_t, visits_table_t, value_table_t, value_table_t,
        tree_walker, choices_t, choices_t, std::mt19937>;

    struct feature_sizer_t : feature_sizer<value_sizer> {
        using feature_sizer<value_sizer>::feature_sizer;
    };
    struct model_sizer_t : model_sizer<feature_sizer_t, value_sizer> {
        using model_sizer<feature_sizer_t, value_sizer>::model_sizer;
    };
    struct primitive_dispatcher_t : primitive_dispatcher<primitive> {
        using primitive_dispatcher<primitive>::primitive_dispatcher;
    };
    struct column_applier_t : column_applier<primitive_dispatcher_t> {
        using column_applier<primitive_dispatcher_t>::column_applier;
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
    struct feature_synthesizer_t : feature_synthesizer<
        cheapest_feature_index, cheapest_feature_index, cheapest_feature_index, variable_candidate_finder_t,
        constant_store, primitive_dispatcher_t, primitive_dispatcher_t, argument_tuple_enumerator, column_applier_t,
        uniform_value_finder, feature_pool, feature_sizer_t> {
        using feature_synthesizer<
            cheapest_feature_index, cheapest_feature_index, cheapest_feature_index, variable_candidate_finder_t,
            constant_store, primitive_dispatcher_t, primitive_dispatcher_t, argument_tuple_enumerator,
            column_applier_t, uniform_value_finder, feature_pool, feature_sizer_t>::feature_synthesizer;
    };
    struct leaf_detector_t : leaf_detector<uniform_value_finder> {
        using leaf_detector<uniform_value_finder>::leaf_detector;
    };
    struct bin_splitter_t : bin_splitter<schema, schema> {
        using bin_splitter<schema, schema>::bin_splitter;
    };
    struct candidate_chooser_t : mcts_candidate_chooser<mcts_t::chooser_t> {
        using mcts_candidate_chooser<mcts_t::chooser_t>::mcts_candidate_chooser;
    };
    struct model_builder_t : model_builder<
        leaf_detector_t, feature_synthesizer_t, candidate_chooser_t, bin_splitter_t,
        variable_candidate_finder_t, candidate_chooser_t, model_node_pool, model_node_pool, model_node_pool> {
        using model_builder<
            leaf_detector_t, feature_synthesizer_t, candidate_chooser_t, bin_splitter_t,
            variable_candidate_finder_t, candidate_chooser_t, model_node_pool, model_node_pool,
            model_node_pool>::model_builder;
    };
    struct model_episode_t : model_episode<model_builder_t, model_sizer_t, best_model_tracker> {
        using model_episode<model_builder_t, model_sizer_t, best_model_tracker>::model_episode;
    };
    struct mcts_episode_t : mcts_episode<model_episode_t, mcts_t::delta_t, mcts_t::terminator_t> {
        using mcts_episode<model_episode_t, mcts_t::delta_t, mcts_t::terminator_t>::mcts_episode;
    };
    struct feature_evaluator_t : feature_evaluator<primitive_dispatcher_t> {
        using feature_evaluator<primitive_dispatcher_t>::feature_evaluator;
    };
    struct model_evaluator_t : model_evaluator<feature_evaluator_t, schema> {
        using model_evaluator<feature_evaluator_t, schema>::model_evaluator;
    };

    razor_manifest(
        schema& types, value_pool& values, const std::vector<primitive>& primitives, const razor_options& options);

    schema& schema_;
    value_pool& values_;
    feature_pool features_;
    model_node_pool nodes_;
    constant_store constants_;
    cheapest_feature_index candidate_index_;
    uniform_value_finder uniform_value_finder_;
    value_sizer value_sizer_;
    feature_sizer_t feature_sizer_;
    model_sizer_t model_sizer_;
    primitive_dispatcher_t primitive_dispatcher_;
    column_applier_t column_applier_;
    argument_tuple_enumerator argument_tuple_enumerator_;
    variable_candidate_finder_t variable_candidate_finder_;
    constant_enumerator_t constant_enumerator_;
    constant_seeder_t constant_seeder_;
    feature_synthesizer_t feature_synthesizer_;
    leaf_detector_t leaf_detector_;
    bin_splitter_t bin_splitter_;
    visits_table_t visits_;
    value_table_t rewards_;
    std::mt19937 random_;
    mcts_t mcts_;
    candidate_chooser_t candidate_chooser_;
    model_builder_t model_builder_;
    best_model_tracker best_model_tracker_;
    model_episode_t model_episode_;
    mcts_episode_t mcts_episode_;
    feature_evaluator_t feature_evaluator_;
    model_evaluator_t model_evaluator_;
};

#endif
