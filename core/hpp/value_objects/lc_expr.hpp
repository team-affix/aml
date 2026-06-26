#ifndef LC_EXPR_HPP
#define LC_EXPR_HPP

#include <cstdint>
#include <variant>

struct lc_expr {
    struct var {
        uint32_t index;
        auto operator<=>(const var&) const = default;
    };
    struct abs {
        const lc_expr* body;
        auto operator<=>(const abs&) const = default;
    };
    struct app {
        const lc_expr* func;
        const lc_expr* arg;
        auto operator<=>(const app&) const = default;
    };
    std::variant<var, abs, app> content;
    auto operator<=>(const lc_expr&) const = default;
};

#endif
