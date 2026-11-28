#ifndef QUARTZ_MANIFEST_HPP
#define QUARTZ_MANIFEST_HPP

#include <cstddef>
#include <random>
#include <vector>
#include "infrastructure/adding_expander.hpp"
#include "infrastructure/ancestry_finder.hpp"
#include "infrastructure/ancestry_sizer.hpp"
#include "infrastructure/argument_type_index.hpp"
#include "infrastructure/best_table_tracker.hpp"
#include "infrastructure/cached_column_maker.hpp"
#include "infrastructure/cheapest_set_visitor.hpp"
#include "infrastructure/child_pusher.hpp"
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
#include "infrastructure/derivation_sampler.hpp"
#include "infrastructure/distinct_row_counter.hpp"
#include "infrastructure/droppable_member_finder.hpp"
#include "infrastructure/dropping_expander.hpp"
#include "infrastructure/eligible_primitive_finder.hpp"
#include "infrastructure/equal_feature_finder.hpp"
#include "infrastructure/feature_deriver.hpp"
#include "infrastructure/feature_graph.hpp"
#include "infrastructure/feature_recorder.hpp"
#include "infrastructure/graph_row_evaluator.hpp"
#include "infrastructure/input_feature_seeder.hpp"
#include "infrastructure/key_drawer.hpp"
#include "infrastructure/model_graph_builder.hpp"
#include "infrastructure/native_primitive_store.hpp"
#include "infrastructure/native_type_store.hpp"
#include "infrastructure/operand_pool_finder.hpp"
#include "infrastructure/origin_replacer.hpp"
#include "infrastructure/row_key_numberer.hpp"
#include "infrastructure/set_key_measurer.hpp"
#include "infrastructure/set_key_store.hpp"
#include "infrastructure/set_queue.hpp"
#include "infrastructure/table_model_builder.hpp"
#include "infrastructure/table_model_evaluator.hpp"
#include "infrastructure/uniform_column_detector.hpp"
#include "infrastructure/value_class_finder.hpp"
#include "value_objects/native_bin.hpp"
#include "value_objects/native_constant.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/native_type.hpp"
#include "value_objects/quartz_options.hpp"

namespace AML {

// Quartz: best-first search over sets of features that are functions of the labels, cheapest first. A visited set
// pushes every legal single drop and up to adds_per_feature sampled additions per feature in its ancestry, each with
// at least one feature operand. Features live in a graph that keeps them for good while their columns, native values
// erased behind their types' operations, live in a cache within a byte budget; constants live in a graph of their own,
// and their values, one each, in a cache of their own.
struct quartz_manifest {
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
    struct set_key_measurer_t : set_key_measurer<feature_graph> {
        using set_key_measurer<feature_graph>::set_key_measurer;
    };
    struct key_drawer_t : key_drawer<std::mt19937_64> {
        using key_drawer<std::mt19937_64>::key_drawer;
    };
    struct child_pusher_t : child_pusher<
        set_key_measurer_t, ancestry_sizer_t, set_key_store, distinct_row_counter_t, set_queue> {
        using child_pusher<
            set_key_measurer_t, ancestry_sizer_t, set_key_store, distinct_row_counter_t, set_queue>::child_pusher;
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
    struct derivation_sampler_t : derivation_sampler<native_primitive_store, constant_graph, std::mt19937_64> {
        using derivation_sampler<native_primitive_store, constant_graph, std::mt19937_64>::derivation_sampler;
    };
    struct dropping_expander_t : dropping_expander<droppable_member_finder_t, child_pusher_t> {
        using dropping_expander<droppable_member_finder_t, child_pusher_t>::dropping_expander;
    };
    struct adding_expander_t : adding_expander<
        ancestry_finder_t, operand_pool_finder_t, eligible_primitive_finder_t, derivation_sampler_t, feature_deriver_t,
        child_pusher_t> {
        using adding_expander<
            ancestry_finder_t, operand_pool_finder_t, eligible_primitive_finder_t, derivation_sampler_t,
            feature_deriver_t, child_pusher_t>::adding_expander;
    };
    struct model_graph_builder_t : model_graph_builder<feature_graph> {
        using model_graph_builder<feature_graph>::model_graph_builder;
    };
    struct table_model_builder_t : table_model_builder<model_graph_builder_t, column_reader_t, native_type_store> {
        using table_model_builder<model_graph_builder_t, column_reader_t, native_type_store>::table_model_builder;
    };
    struct cheapest_set_visitor_t : cheapest_set_visitor<
        set_queue, table_model_builder_t, best_table_tracker, dropping_expander_t, adding_expander_t> {
        using cheapest_set_visitor<
            set_queue, table_model_builder_t, best_table_tracker, dropping_expander_t,
            adding_expander_t>::cheapest_set_visitor;
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

    quartz_manifest(
        const native_bin& training,
        const std::vector<native_constant>& constants,
        const std::vector<native_type>& types,
        const std::vector<native_primitive>& primitives,
        const quartz_options& options);

    std::mt19937_64             random_;
    row_key_numberer            row_key_numberer_;
    uniform_column_detector     uniform_column_detector_;
    native_type_store           native_types_;
    native_primitive_store      native_primitives_;
    argument_type_index         argument_types_;
    std::vector<size_t>         label_classes_;
    feature_graph               feature_graph_;
    constant_graph              constant_graph_;
    derivation_index            derivation_index_;
    column_hash_index           column_hash_index_;
    column_cache                column_cache_;
    constant_cache              constant_cache_;
    set_key_store               set_keys_;
    set_queue                   set_queue_;
    best_table_tracker          best_table_tracker_;
    column_computer_t           column_computer_;
    cached_column_maker_t       cached_column_maker_;
    column_byte_measurer_t      column_byte_measurer_;
    constant_computer_t         constant_computer_;
    constant_byte_measurer_t    constant_byte_measurer_;
    constant_reader_t           constant_reader_;
    column_reader_t             column_reader_;
    ancestry_finder_t           ancestry_finder_;
    constant_ancestry_finder_t  constant_ancestry_finder_;
    ancestry_sizer_t            ancestry_sizer_;
    distinct_row_counter_t      distinct_row_counter_;
    droppable_member_finder_t   droppable_member_finder_;
    set_key_measurer_t          set_key_measurer_;
    key_drawer_t                key_drawer_;
    child_pusher_t              child_pusher_;
    equal_feature_finder_t      equal_feature_finder_;
    origin_replacer_t           origin_replacer_;
    feature_recorder_t          feature_recorder_;
    feature_deriver_t           feature_deriver_;
    operand_pool_finder_t       operand_pool_finder_;
    eligible_primitive_finder_t eligible_primitive_finder_;
    derivation_sampler_t        derivation_sampler_;
    dropping_expander_t         dropping_expander_;
    adding_expander_t           adding_expander_;
    model_graph_builder_t       model_graph_builder_;
    table_model_builder_t       table_model_builder_;
    cheapest_set_visitor_t      cheapest_set_visitor_;
    input_feature_seeder_t      input_feature_seeder_;
    constant_atom_seeder_t      constant_atom_seeder_;
    graph_row_evaluator_t       graph_row_evaluator_;
    value_class_finder_t        value_class_finder_;
    table_model_evaluator_t     table_model_evaluator_;
};

}

#endif
