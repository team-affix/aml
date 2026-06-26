#ifndef TRANSPILER_HPP
#define TRANSPILER_HPP

#include <map>
#include <optional>
#include <string>
#include <vector>
#include "infrastructure/lc_expr_pool.hpp"
#include "value_objects/aml_program.hpp"

struct transpiled_function {
    std::string     name;
    const lc_expr*  body;
};

struct transpiled_program {
    lc_expr_pool                  pool;
    std::map<std::string, const lc_expr*> globals;
    std::vector<transpiled_function>      functions;

    transpiled_program() = default;
    transpiled_program(const transpiled_program&) = delete;
    transpiled_program& operator=(const transpiled_program&) = delete;
    transpiled_program(transpiled_program&&) = default;
    transpiled_program& operator=(transpiled_program&&) = default;
};

struct transpiler {
    explicit transpiler(lc_expr_pool& pool);

    const lc_expr* transpile(const aml_expr* e);
    const lc_expr* transpile(const aml_expr* e, const std::map<std::string, const lc_expr*>& globals);

    void register_builtins(std::map<std::string, const lc_expr*>& globals);
    void register_group(const constructor_group& group, std::map<std::string, const lc_expr*>& globals);

    static transpiled_program transpile_program(const aml_program& prog);

private:
    struct binding_env {
        std::vector<std::string> stack;

        void push(std::string name) { stack.push_back(std::move(name)); }
        void pop() { stack.pop_back(); }
        std::optional<uint32_t> lookup(const std::string& name) const;
    };

    lc_expr_pool& pool_;
    const std::map<std::string, const lc_expr*>* globals_ = nullptr;

    const lc_expr* scott_constructor(uint32_t index, uint32_t group_size, uint32_t arity);
    const lc_expr* scott_nat(uint64_t value);
    const lc_expr* church_nat(uint64_t value);
    const lc_expr* scott_list(const std::vector<const lc_expr*>& elems);
    const lc_expr* church_list(const std::vector<const lc_expr*>& elems);

    const lc_expr* transpile_var(const aml_expr::var& v, const binding_env& env);
    const lc_expr* transpile_abs(const aml_expr::abs& a, const binding_env& env);
    const lc_expr* transpile_app(const aml_expr::app& a, const binding_env& env);
    const lc_expr* transpile_nat(const aml_expr::nat& n);
    const lc_expr* transpile_integer(const aml_expr::integer& i);
    const lc_expr* transpile_character(const aml_expr::character& c);
    const lc_expr* transpile_string(const aml_expr::string& s);
    const lc_expr* transpile_list(const aml_expr::list& l, const binding_env& env);

    const lc_expr* transpile(const aml_expr* e, const binding_env& env);
};

// Test / debug helper: structural equality of lc_expr trees.
bool lc_expr_eq(const lc_expr* a, const lc_expr* b);

#endif
