#ifndef MCTS_CANDIDATE_CHOOSER_HPP
#define MCTS_CANDIDATE_CHOOSER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/mcts_choice.hpp"
#include "debug_assert.hpp"

// Chooses a candidate by asking MCTS to choose among the candidates' indices.
template<typename IChooseChoice>
struct mcts_candidate_chooser {
    mcts_candidate_chooser(IChooseChoice& choose_choice);
    size_t choose(const std::vector<candidate>& candidates);
private:
    IChooseChoice& choose_choice_;
};

#define MCTS_CANDIDATE_CHOOSER_TEMPLATE template<typename IChooseChoice>
#define MCTS_CANDIDATE_CHOOSER          mcts_candidate_chooser<IChooseChoice>

MCTS_CANDIDATE_CHOOSER_TEMPLATE
MCTS_CANDIDATE_CHOOSER::mcts_candidate_chooser(IChooseChoice& choose_choice)
    : choose_choice_(choose_choice) {}

MCTS_CANDIDATE_CHOOSER_TEMPLATE
size_t MCTS_CANDIDATE_CHOOSER::choose(const std::vector<candidate>& candidates) {
    DEBUG_ASSERT(!candidates.empty());
    std::vector<mcts_choice> choices;
    for (mcts_choice index = 0; index < candidates.size(); ++index)
        choices.push_back(index);
    return choose_choice_.choose(choices, choices);
}

#undef MCTS_CANDIDATE_CHOOSER
#undef MCTS_CANDIDATE_CHOOSER_TEMPLATE

#endif
