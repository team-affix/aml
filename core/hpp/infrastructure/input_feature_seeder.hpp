#ifndef INPUT_FEATURE_SEEDER_HPP
#define INPUT_FEATURE_SEEDER_HPP

#include <cstddef>
#include <memory>
#include <vector>
#include "value_objects/cached_column.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/native_column.hpp"
#include "value_objects/var_id.hpp"

namespace AML {

// Seeds the graph with every input as a feature, in order, and gives the inputs whose columns hold more than one value:
// the set the search starts from.
template<
    typename IComputeColumn, typename IMakeCachedColumn, typename IRecordFeature, typename IDetectUniformColumn>
struct input_feature_seeder {
    input_feature_seeder(
        IComputeColumn& compute_column,
        IMakeCachedColumn& make_cached_column,
        IRecordFeature& record_feature,
        IDetectUniformColumn& detect_uniform_column);
    std::vector<feature_id> seed(const std::vector<native_column>& inputs);
private:
    IComputeColumn& compute_column_;
    IMakeCachedColumn& make_cached_column_;
    IRecordFeature& record_feature_;
    IDetectUniformColumn& detect_uniform_column_;
};

#define INPUT_FEATURE_SEEDER_TEMPLATE \
    template< \
        typename IComputeColumn, typename IMakeCachedColumn, typename IRecordFeature, typename IDetectUniformColumn>
#define INPUT_FEATURE_SEEDER \
    input_feature_seeder<IComputeColumn, IMakeCachedColumn, IRecordFeature, IDetectUniformColumn>

INPUT_FEATURE_SEEDER_TEMPLATE
INPUT_FEATURE_SEEDER::input_feature_seeder(
    IComputeColumn& compute_column,
    IMakeCachedColumn& make_cached_column,
    IRecordFeature& record_feature,
    IDetectUniformColumn& detect_uniform_column)
    : compute_column_(compute_column)
    , make_cached_column_(make_cached_column)
    , record_feature_(record_feature)
    , detect_uniform_column_(detect_uniform_column) {}

INPUT_FEATURE_SEEDER_TEMPLATE
std::vector<feature_id> INPUT_FEATURE_SEEDER::seed(const std::vector<native_column>& inputs) {
    std::vector<feature_id> varied;
    for (size_t index = 0; index < inputs.size(); ++index) {
        const derivation origin{derivation::input{.index = static_cast<var_id>(index)}};
        const std::shared_ptr<const cached_column> made =
            make_cached_column_.make(inputs[index].type, compute_column_.compute(origin, {}));
        const bool uniform = detect_uniform_column_.detect(*made);
        const feature_id feature = record_feature_.record(origin, made);
        if (uniform)
            continue;
        varied.push_back(feature);
    }
    return varied;
}

#undef INPUT_FEATURE_SEEDER
#undef INPUT_FEATURE_SEEDER_TEMPLATE

}

#endif
