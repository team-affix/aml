#ifndef FEATURE_RECORDER_HPP
#define FEATURE_RECORDER_HPP

#include <memory>
#include "value_objects/cached_column.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"

namespace AML {

// Records a new feature with its column: adds it to the graph with a fresh key, indexes its derivation and its
// column's hash, and caches its column.
template<
    typename IDrawKey, typename IInsertFeatureRecord, typename IInsertDerivation, typename IInsertColumnHash,
    typename IMeasureColumnBytes, typename IInsertCachedColumn>
struct feature_recorder {
    feature_recorder(
        IDrawKey& draw_key,
        IInsertFeatureRecord& insert_feature_record,
        IInsertDerivation& insert_derivation,
        IInsertColumnHash& insert_column_hash,
        IMeasureColumnBytes& measure_column_bytes,
        IInsertCachedColumn& insert_cached_column);
    feature_id record(const derivation& origin, std::shared_ptr<const cached_column> made);
private:
    IDrawKey& draw_key_;
    IInsertFeatureRecord& insert_feature_record_;
    IInsertDerivation& insert_derivation_;
    IInsertColumnHash& insert_column_hash_;
    IMeasureColumnBytes& measure_column_bytes_;
    IInsertCachedColumn& insert_cached_column_;
};

#define FEATURE_RECORDER_TEMPLATE \
    template< \
        typename IDrawKey, typename IInsertFeatureRecord, typename IInsertDerivation, typename IInsertColumnHash, \
        typename IMeasureColumnBytes, typename IInsertCachedColumn>
#define FEATURE_RECORDER \
    feature_recorder< \
        IDrawKey, IInsertFeatureRecord, IInsertDerivation, IInsertColumnHash, IMeasureColumnBytes, \
        IInsertCachedColumn>

FEATURE_RECORDER_TEMPLATE
FEATURE_RECORDER::feature_recorder(
    IDrawKey& draw_key,
    IInsertFeatureRecord& insert_feature_record,
    IInsertDerivation& insert_derivation,
    IInsertColumnHash& insert_column_hash,
    IMeasureColumnBytes& measure_column_bytes,
    IInsertCachedColumn& insert_cached_column)
    : draw_key_(draw_key)
    , insert_feature_record_(insert_feature_record)
    , insert_derivation_(insert_derivation)
    , insert_column_hash_(insert_column_hash)
    , measure_column_bytes_(measure_column_bytes)
    , insert_cached_column_(insert_cached_column) {}

FEATURE_RECORDER_TEMPLATE
feature_id FEATURE_RECORDER::record(const derivation& origin, std::shared_ptr<const cached_column> made) {
    const feature_id feature = insert_feature_record_.insert(
        feature_record{.origin = origin, .type = made->type, .key = draw_key_.draw()});
    insert_derivation_.insert(origin, feature);
    insert_column_hash_.insert(made->hash, feature);
    const size_t bytes = measure_column_bytes_.measure(*made);
    insert_cached_column_.insert(feature, std::move(made), bytes);
    return feature;
}

#undef FEATURE_RECORDER
#undef FEATURE_RECORDER_TEMPLATE

}

#endif
