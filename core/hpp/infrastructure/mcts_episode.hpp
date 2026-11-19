#ifndef MCTS_EPISODE_HPP
#define MCTS_EPISODE_HPP

#include <cstddef>
#include "value_objects/bin.hpp"

// Runs one episode and rewards the MCTS path it took with the negated model cost, then ends the MCTS episode.
// Returns the model's cost.
template<typename IRunEpisode, typename ISetReward, typename ITerminate>
struct mcts_episode {
    mcts_episode(IRunEpisode& run_episode, ISetReward& set_reward, ITerminate& terminate);
    size_t run(const bin& training);
private:
    IRunEpisode& run_episode_;
    ISetReward& set_reward_;
    ITerminate& terminate_;
};

#define MCTS_EPISODE_TEMPLATE template<typename IRunEpisode, typename ISetReward, typename ITerminate>
#define MCTS_EPISODE          mcts_episode<IRunEpisode, ISetReward, ITerminate>

MCTS_EPISODE_TEMPLATE
MCTS_EPISODE::mcts_episode(IRunEpisode& run_episode, ISetReward& set_reward, ITerminate& terminate)
    : run_episode_(run_episode)
    , set_reward_(set_reward)
    , terminate_(terminate) {}

MCTS_EPISODE_TEMPLATE
size_t MCTS_EPISODE::run(const bin& training) {
    const size_t cost = run_episode_.run(training);
    set_reward_.set_value(-static_cast<double>(cost));
    terminate_.terminate();
    return cost;
}

#undef MCTS_EPISODE
#undef MCTS_EPISODE_TEMPLATE

#endif
