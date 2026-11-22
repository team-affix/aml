#include "infrastructure/scrutinee_piece_finder.hpp"

#include <variant>

namespace AML {

std::vector<feature> scrutinee_piece_finder::find(const feature& scrutinee) const {
    std::vector<feature> pieces;
    collect(scrutinee, pieces);
    return pieces;
}

void scrutinee_piece_finder::collect(const feature& expression, std::vector<feature>& pieces) const {
    const feature::app* application = std::get_if<feature::app>(&expression.content);
    if (application == nullptr)
        return;
    bool held = false;
    for (const feature& earlier : pieces)
        held = held || earlier == expression;
    if (!held)
        pieces.push_back(expression);
    for (const feature& argument : application->args)
        collect(argument, pieces);
}

}
