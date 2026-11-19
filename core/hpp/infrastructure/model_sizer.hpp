#ifndef MODEL_SIZER_HPP
#define MODEL_SIZER_HPP

#include <cstddef>
#include <variant>
#include "value_objects/model_node.hpp"

// Sizes a model: a match costs one plus its scrutinee's size, a leaf costs its output's size, and empty costs nothing.
template<typename ISizeFeature>
struct model_sizer {
    model_sizer(ISizeFeature& size_feature);
    size_t size(const model_node& node) const;
private:
    ISizeFeature& size_feature_;
};

#define MODEL_SIZER_TEMPLATE template<typename ISizeFeature>
#define MODEL_SIZER          model_sizer<ISizeFeature>

MODEL_SIZER_TEMPLATE
MODEL_SIZER::model_sizer(ISizeFeature& size_feature)
    : size_feature_(size_feature) {}

MODEL_SIZER_TEMPLATE
size_t MODEL_SIZER::size(const model_node& node) const {
    if (std::holds_alternative<model_node::empty>(node.content))
        return 0;
    if (const model_node::leaf* leaf = std::get_if<model_node::leaf>(&node.content))
        return size_feature_.size(leaf->output);
    const model_node::match& match = std::get<model_node::match>(node.content);
    size_t total = 1 + size_feature_.size(match.scrutinee);
    for (const model_node& branch : match.branches)
        total += size(branch);
    return total;
}

#undef MODEL_SIZER
#undef MODEL_SIZER_TEMPLATE

#endif
