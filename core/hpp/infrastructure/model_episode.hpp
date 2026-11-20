#ifndef MODEL_EPISODE_HPP
#define MODEL_EPISODE_HPP

#include <cstddef>
#include <utility>
#include "value_objects/bin.hpp"
#include "value_objects/episode_outcome.hpp"
#include "value_objects/model_node.hpp"

// Runs one episode: builds a model of the training bin, sizes it and offers it as the best model. Returns the model
// and its cost.
template<typename IBuildModel, typename ISizeModel, typename ITryRecordModel>
struct model_episode {
    model_episode(IBuildModel& build_model, ISizeModel& size_model, ITryRecordModel& try_record_model);
    episode_outcome run(const bin& training);
private:
    IBuildModel& build_model_;
    ISizeModel& size_model_;
    ITryRecordModel& try_record_model_;
};

#define MODEL_EPISODE_TEMPLATE template<typename IBuildModel, typename ISizeModel, typename ITryRecordModel>
#define MODEL_EPISODE          model_episode<IBuildModel, ISizeModel, ITryRecordModel>

MODEL_EPISODE_TEMPLATE
MODEL_EPISODE::model_episode(IBuildModel& build_model, ISizeModel& size_model, ITryRecordModel& try_record_model)
    : build_model_(build_model)
    , size_model_(size_model)
    , try_record_model_(try_record_model) {}

MODEL_EPISODE_TEMPLATE
episode_outcome MODEL_EPISODE::run(const bin& training) {
    model_node model = build_model_.build(training);
    const size_t cost = size_model_.size(model);
    try_record_model_.try_record(model, cost);
    return episode_outcome{.model = std::move(model), .cost = cost};
}

#undef MODEL_EPISODE
#undef MODEL_EPISODE_TEMPLATE

#endif
