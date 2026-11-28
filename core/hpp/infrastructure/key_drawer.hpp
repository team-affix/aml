#ifndef KEY_DRAWER_HPP
#define KEY_DRAWER_HPP

#include <cstdint>
#include <random>

namespace AML {

// Draws a key uniformly from every 64-bit value with the injected random engine.
template<typename IRandomEngine>
struct key_drawer {
    key_drawer(IRandomEngine& random_engine);
    uint64_t draw();
private:
    IRandomEngine& random_engine_;
};

#define KEY_DRAWER_TEMPLATE template<typename IRandomEngine>
#define KEY_DRAWER          key_drawer<IRandomEngine>

KEY_DRAWER_TEMPLATE
KEY_DRAWER::key_drawer(IRandomEngine& random_engine)
    : random_engine_(random_engine) {}

KEY_DRAWER_TEMPLATE
uint64_t KEY_DRAWER::draw() {
    std::uniform_int_distribution<uint64_t> keys;
    return keys(random_engine_);
}

#undef KEY_DRAWER
#undef KEY_DRAWER_TEMPLATE

}

#endif
