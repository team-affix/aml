#ifndef SCOUT_MANIFEST_HPP
#define SCOUT_MANIFEST_HPP

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
#include "infrastructure/first_candidate_chooser.hpp"
#include "infrastructure/leaf_detector.hpp"
#include "infrastructure/model_builder.hpp"
#include "infrastructure/model_episode.hpp"
#include "infrastructure/model_evaluator.hpp"
#include "infrastructure/model_node_pool.hpp"
#include "infrastructure/model_sizer.hpp"
#include "infrastructure/primitive.hpp"
#include "infrastructure/primitive_dispatcher.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/uniform_value_finder.hpp"
#include "infrastructure/value_pool.hpp"
#include "infrastructure/value_sizer.hpp"
#include "infrastructure/variable_candidate_finder.hpp"
#include "value_objects/scout_options.hpp"

// Scout: builds one model, taking the first candidate at every choice.
struct scout_manifest {
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
    struct model_builder_t : model_builder<
        leaf_detector_t, feature_synthesizer_t, first_candidate_chooser, bin_splitter_t,
        variable_candidate_finder_t, first_candidate_chooser, model_node_pool, model_node_pool, model_node_pool> {
        using model_builder<
            leaf_detector_t, feature_synthesizer_t, first_candidate_chooser, bin_splitter_t,
            variable_candidate_finder_t, first_candidate_chooser, model_node_pool, model_node_pool,
            model_node_pool>::model_builder;
    };
    struct model_episode_t : model_episode<model_builder_t, model_sizer_t, best_model_tracker> {
        using model_episode<model_builder_t, model_sizer_t, best_model_tracker>::model_episode;
    };
    struct feature_evaluator_t : feature_evaluator<primitive_dispatcher_t> {
        using feature_evaluator<primitive_dispatcher_t>::feature_evaluator;
    };
    struct model_evaluator_t : model_evaluator<feature_evaluator_t, schema> {
        using model_evaluator<feature_evaluator_t, schema>::model_evaluator;
    };

    scout_manifest(
        schema& types, value_pool& values, const std::vector<primitive>& primitives, const scout_options& options);

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
    first_candidate_chooser candidate_chooser_;
    model_builder_t model_builder_;
    best_model_tracker best_model_tracker_;
    model_episode_t model_episode_;
    feature_evaluator_t feature_evaluator_;
    model_evaluator_t model_evaluator_;
};

#endif
