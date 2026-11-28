#include "infrastructure/quartz_manifest.hpp"

namespace AML {

quartz_manifest::quartz_manifest(
    const native_bin& training,
    const std::vector<native_constant>& constants,
    const std::vector<native_type>& types,
    const std::vector<native_primitive>& primitives,
    const quartz_options& options)
    : random_(options.seed)
    , row_key_numberer_()
    , uniform_column_detector_()
    , native_types_(types)
    , native_primitives_(primitives)
    , argument_types_(primitives)
    , label_classes_(native_types_.type(training.labels.type).classify(training.labels.cells))
    , feature_graph_()
    , constant_graph_()
    , derivation_index_()
    , column_hash_index_()
    , column_cache_(options.column_cache_bytes)
    , constant_cache_(options.constant_cache_bytes)
    , set_keys_()
    , set_queue_(options.queue_capacity)
    , best_table_tracker_()
    , column_computer_(native_primitives_, training.inputs, training.row_count)
    , cached_column_maker_(native_types_)
    , column_byte_measurer_(native_types_)
    , constant_computer_(native_primitives_, native_types_, constants)
    , constant_byte_measurer_(native_types_)
    , constant_reader_(
          constant_graph_, constant_cache_, constant_computer_, constant_byte_measurer_, constant_cache_)
    , column_reader_(
          feature_graph_, constant_reader_, column_cache_, column_computer_, cached_column_maker_,
          column_byte_measurer_, column_cache_)
    , ancestry_finder_(feature_graph_)
    , constant_ancestry_finder_(constant_graph_)
    , ancestry_sizer_(ancestry_finder_, feature_graph_, constant_ancestry_finder_)
    , distinct_row_counter_(column_reader_, row_key_numberer_, training.row_count)
    , droppable_member_finder_(column_reader_, row_key_numberer_, label_classes_)
    , set_key_measurer_(feature_graph_)
    , key_drawer_(random_)
    , child_pusher_(set_key_measurer_, ancestry_sizer_, set_keys_, distinct_row_counter_, set_queue_)
    , equal_feature_finder_(column_hash_index_, column_reader_, native_types_)
    , origin_replacer_(ancestry_sizer_, feature_graph_)
    , feature_recorder_(
          key_drawer_, feature_graph_, derivation_index_, column_hash_index_, column_byte_measurer_, column_cache_)
    , feature_deriver_(
          derivation_index_, column_reader_, constant_reader_, column_computer_, native_primitives_,
          cached_column_maker_, uniform_column_detector_, equal_feature_finder_, origin_replacer_, derivation_index_,
          feature_recorder_)
    , operand_pool_finder_(feature_graph_)
    , eligible_primitive_finder_(argument_types_)
    , derivation_sampler_(native_primitives_, constant_graph_, random_)
    , dropping_expander_(droppable_member_finder_, child_pusher_)
    , adding_expander_(
          ancestry_finder_, operand_pool_finder_, eligible_primitive_finder_, derivation_sampler_, feature_deriver_,
          child_pusher_, options.adds_per_feature)
    , model_graph_builder_(feature_graph_)
    , table_model_builder_(
          model_graph_builder_, column_reader_, native_types_, training.labels, training.row_count)
    , cheapest_set_visitor_(
          set_queue_, table_model_builder_, best_table_tracker_, dropping_expander_, adding_expander_)
    , input_feature_seeder_(column_computer_, cached_column_maker_, feature_recorder_, uniform_column_detector_)
    , constant_atom_seeder_(constant_graph_)
    , graph_row_evaluator_(native_primitives_, native_types_, constant_reader_)
    , value_class_finder_(native_types_)
    , table_model_evaluator_(graph_row_evaluator_, value_class_finder_) {}

}
