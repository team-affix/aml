#ifndef AML_EXPR_HPP
#define AML_EXPR_HPP

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

struct aml_expr {
    struct app {
        const aml_expr* func;
        const aml_expr* arg;
        auto operator<=>(const app&) const = default;
    };
    struct abs {
        std::string     param;
        const aml_expr* body;
        auto operator<=>(const abs&) const = default;
    };
    struct var {
        std::string name;
        auto operator<=>(const var&) const = default;
    };
    struct nat {
        uint64_t value;
        bool     church;
        auto operator<=>(const nat&) const = default;
    };
    struct integer {
        int64_t value;
        auto operator<=>(const integer&) const = default;
    };
    struct character {
        uint32_t codepoint;
        auto operator<=>(const character&) const = default;
    };
    struct string {
        std::string value;
        auto operator<=>(const string&) const = default;
    };
    struct list {
        std::vector<const aml_expr*> elems;
        bool                         church;
        auto operator<=>(const list&) const = default;
    };
    std::variant<app, abs, var, nat, integer, character, string, list> content;
    auto operator<=>(const aml_expr&) const = default;
};

#endif
