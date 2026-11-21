#ifndef MCTS_EPISODE_HPP
#define MCTS_EPISODE_HPP

#include <cstddef>
#include "value_objects/bin_view.hpp"
#include "value_objects/episode_outcome.hpp"

namespace AML {

// Runs one episode and rewards the MCTS path it took with the negated model cost, then ends the MCTS episode.
// Returns the episode's model and its cost.
template<typename IRunEpisode, typename ISetReward, typename ITerminate>
struct mcts_episode {
    mcts_episode(IRunEpisode& run_episode, ISetReward& set_reward, ITerminate& terminate);
    episode_outcome run(const bin_view& training);
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
episode_outcome MCTS_EPISODE::run(const bin_view& training) {
    episode_outcome outcome = run_episode_.run(training);
    set_reward_.set_value(-static_cast<double>(outcome.cost));
    terminate_.terminate();
    return outcome;
}

#undef MCTS_EPISODE
#undef MCTS_EPISODE_TEMPLATE

}

#endif
