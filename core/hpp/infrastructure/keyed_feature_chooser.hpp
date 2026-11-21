#ifndef KEYED_FEATURE_CHOOSER_HPP
#define KEYED_FEATURE_CHOOSER_HPP

#include <cstddef>
#include <vector>

namespace AML {

// Chooses the feature a bin is matched on, among the session's features, by a key measured for each feature's
// position (how recent it is, for instance), so the choice among them can follow the keys.
template<typename IMeasureKey, typename IChooseOption>
struct keyed_feature_chooser {
    keyed_feature_chooser(IMeasureKey& measure_key, IChooseOption& choose_option);
    size_t choose(size_t feature_count);
private:
    IMeasureKey& measure_key_;
    IChooseOption& choose_option_;
};

#define KEYED_FEATURE_CHOOSER_TEMPLATE template<typename IMeasureKey, typename IChooseOption>
#define KEYED_FEATURE_CHOOSER          keyed_feature_chooser<IMeasureKey, IChooseOption>

KEYED_FEATURE_CHOOSER_TEMPLATE
KEYED_FEATURE_CHOOSER::keyed_feature_chooser(IMeasureKey& measure_key, IChooseOption& choose_option)
    : measure_key_(measure_key)
    , choose_option_(choose_option) {}

KEYED_FEATURE_CHOOSER_TEMPLATE
size_t KEYED_FEATURE_CHOOSER::choose(size_t feature_count) {
    std::vector<size_t> keys;
    keys.reserve(feature_count);
    for (size_t position = 0; position < feature_count; ++position)
        keys.push_back(measure_key_.measure(position));
    return choose_option_.choose(keys);
}

#undef KEYED_FEATURE_CHOOSER
#undef KEYED_FEATURE_CHOOSER_TEMPLATE

}

#endif
