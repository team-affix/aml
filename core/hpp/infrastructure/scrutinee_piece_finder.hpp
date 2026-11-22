#ifndef SCRUTINEE_PIECE_FINDER_HPP
#define SCRUTINEE_PIECE_FINDER_HPP

#include <vector>
#include "value_objects/feature.hpp"

namespace AML {

// Finds the pieces a match passes down: its scrutinee and every primitive application inside it, each once, in
// preorder (a piece before its arguments, arguments left to right). A variable or a constant has no pieces. Training
// and inference both read this order, so the variables the pieces become are numbered the same way in both.
struct scrutinee_piece_finder {
    std::vector<feature> find(const feature& scrutinee) const;
private:
    void collect(const feature& expression, std::vector<feature>& pieces) const;
};

}

#endif
