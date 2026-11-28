#ifndef MCTS_WALK_EPISODE_HPP
#define MCTS_WALK_EPISODE_HPP

#include "value_objects/table_outcome.hpp"

namespace AML {

// Runs one walk and rewards the MCTS path it took with the negated cost of the cheapest set it passed through, then
// ends the MCTS episode. Returns that set's model and cost.
template<typename IRunWalk, typename ISetReward, typename ITerminate>
struct mcts_walk_episode {
    mcts_walk_episode(IRunWalk& run_walk, ISetReward& set_reward, ITerminate& terminate);
    table_outcome run();
private:
    IRunWalk& run_walk_;
    ISetReward& set_reward_;
    ITerminate& terminate_;
};

#define MCTS_WALK_EPISODE_TEMPLATE template<typename IRunWalk, typename ISetReward, typename ITerminate>
#define MCTS_WALK_EPISODE          mcts_walk_episode<IRunWalk, ISetReward, ITerminate>

MCTS_WALK_EPISODE_TEMPLATE
MCTS_WALK_EPISODE::mcts_walk_episode(IRunWalk& run_walk, ISetReward& set_reward, ITerminate& terminate)
    : run_walk_(run_walk)
    , set_reward_(set_reward)
    , terminate_(terminate) {}

MCTS_WALK_EPISODE_TEMPLATE
table_outcome MCTS_WALK_EPISODE::run() {
    table_outcome outcome = run_walk_.run();
    set_reward_.set_value(-static_cast<double>(outcome.cost));
    terminate_.terminate();
    return outcome;
}

#undef MCTS_WALK_EPISODE
#undef MCTS_WALK_EPISODE_TEMPLATE

}

#endif
