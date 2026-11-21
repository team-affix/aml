#ifndef FRACTION_DRAWER_HPP
#define FRACTION_DRAWER_HPP

#include <random>

namespace AML {

// Draws a fraction uniformly from [0, 1) with the injected random engine.
template<typename IRandomEngine>
struct fraction_drawer {
    fraction_drawer(IRandomEngine& random_engine);
    double draw();
private:
    IRandomEngine& random_engine_;
};

#define FRACTION_DRAWER_TEMPLATE template<typename IRandomEngine>
#define FRACTION_DRAWER          fraction_drawer<IRandomEngine>

FRACTION_DRAWER_TEMPLATE
FRACTION_DRAWER::fraction_drawer(IRandomEngine& random_engine)
    : random_engine_(random_engine) {}

FRACTION_DRAWER_TEMPLATE
double FRACTION_DRAWER::draw() {
    std::uniform_real_distribution<double> fraction(0.0, 1.0);
    return fraction(random_engine_);
}

#undef FRACTION_DRAWER
#undef FRACTION_DRAWER_TEMPLATE

}

#endif
