#include "infrastructure/transpiler.hpp"
#include <stdexcept>
#include <variant>
#include <vector>

std::optional<uint32_t> transpiler::binding_env::lookup(const std::string& name) const {
    for (size_t i = stack.size(); i > 0; --i) {
        if (stack[i - 1] == name)
            return static_cast<uint32_t>(stack.size() - i);
    }
    return std::nullopt;
}

transpiler::transpiler(lc_expr_pool& pool) : pool_(pool) {}

const lc_expr* transpiler::scott_constructor(uint32_t index, uint32_t group_size, uint32_t arity) {
    if (index >= group_size)
        throw std::runtime_error("constructor index out of range");
    if (group_size == 0)
        throw std::runtime_error("empty constructor group");

    const lc_expr* body = pool_.make_var(group_size - 1 - index);
    for (uint32_t i = 0; i < arity; ++i) {
        uint32_t arg_idx = group_size + arity - 1 - i;
        body = pool_.make_app(body, pool_.make_var(arg_idx));
    }
    for (uint32_t i = 0; i < group_size; ++i)
        body = pool_.make_lam(body);
    for (uint32_t i = 0; i < arity; ++i)
        body = pool_.make_lam(body);
    return body;
}

void transpiler::register_builtins(std::map<std::string, const lc_expr*>& globals) {
    const constructor_group bools{{"true", 0}, {"false", 0}};
    const constructor_group lists{{"cons", 2}, {"nil", 0}};
    const constructor_group ints{{"pos", 1}, {"negsuc", 1}};

    register_group(bools, globals);
    register_group(lists, globals);
    register_group(ints, globals);
}

void transpiler::register_group(const constructor_group& group,
                                std::map<std::string, const lc_expr*>& globals) {
    const uint32_t n = static_cast<uint32_t>(group.size());
    for (uint32_t i = 0; i < n; ++i) {
        const auto& decl = group[i];
        globals[decl.name] = scott_constructor(i, n, decl.arity);
    }
}

const lc_expr* transpiler::scott_nat(uint64_t value) {
    auto it_cons = globals_->find("cons");
    auto it_nil  = globals_->find("nil");
    auto it_true = globals_->find("true");
    auto it_false = globals_->find("false");
    if (it_cons == globals_->end() || it_nil == globals_->end()
        || it_true == globals_->end() || it_false == globals_->end())
        throw std::runtime_error("builtin constructors missing");

    const lc_expr* list = it_nil->second;
    if (value == 0)
        return list;

    std::vector<bool> bits;
    for (uint64_t v = value; v > 0; v >>= 1)
        bits.push_back((v & 1) != 0);
    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        const lc_expr* bit = *it ? it_true->second : it_false->second;
        list = pool_.make_app(pool_.make_app(it_cons->second, bit), list);
    }
    return list;
}

const lc_expr* transpiler::church_nat(uint64_t value) {
    const lc_expr* body = pool_.make_var(0);
    for (uint64_t i = 0; i < value; ++i)
        body = pool_.make_app(pool_.make_var(1), body);
    return pool_.make_lam(pool_.make_lam(body));
}

const lc_expr* transpiler::scott_list(const std::vector<const lc_expr*>& elems) {
    auto it_cons = globals_->find("cons");
    auto it_nil  = globals_->find("nil");
    if (it_cons == globals_->end() || it_nil == globals_->end())
        throw std::runtime_error("builtin list constructors missing");

    const lc_expr* list = it_nil->second;
    for (auto it = elems.rbegin(); it != elems.rend(); ++it)
        list = pool_.make_app(pool_.make_app(it_cons->second, *it), list);
    return list;
}

const lc_expr* transpiler::church_list(const std::vector<const lc_expr*>& elems) {
    const lc_expr* body = pool_.make_var(0);
    for (auto it = elems.rbegin(); it != elems.rend(); ++it)
        body = pool_.make_app(pool_.make_app(pool_.make_var(1), *it), body);
    return pool_.make_lam(pool_.make_lam(body));
}

const lc_expr* transpiler::transpile_nat(const aml_expr::nat& n) {
    return n.church ? church_nat(n.value) : scott_nat(n.value);
}

const lc_expr* transpiler::transpile_integer(const aml_expr::integer& i) {
    auto it_pos = globals_->find("pos");
    auto it_negsuc = globals_->find("negsuc");
    if (it_pos == globals_->end() || it_negsuc == globals_->end())
        throw std::runtime_error("builtin integer constructors missing");

    if (i.value > 0)
        return pool_.make_app(it_pos->second, scott_nat(static_cast<uint64_t>(i.value)));
    if (i.value < 0)
        return pool_.make_app(it_negsuc->second,
                              scott_nat(static_cast<uint64_t>(-i.value - 1)));
    return pool_.make_app(it_pos->second, scott_nat(0));
}

const lc_expr* transpiler::transpile_character(const aml_expr::character& c) {
    aml_expr::integer i{static_cast<int64_t>(c.codepoint)};
    return transpile_integer(i);
}

const lc_expr* transpiler::transpile_string(const aml_expr::string& s) {
    std::vector<const lc_expr*> elems;
    elems.reserve(s.value.size());
    for (unsigned char ch : s.value) {
        aml_expr::character c{static_cast<uint32_t>(ch)};
        elems.push_back(transpile_character(c));
    }
    return scott_list(elems);
}

const lc_expr* transpiler::transpile_var(const aml_expr::var& v, const binding_env& env) {
    if (auto idx = env.lookup(v.name))
        return pool_.make_var(*idx);
    auto it = globals_->find(v.name);
    if (it != globals_->end())
        return it->second;
    throw std::runtime_error("unbound name: " + v.name);
}

const lc_expr* transpiler::transpile_abs(const aml_expr::abs& a, const binding_env& env) {
    binding_env inner = env;
    inner.push(a.param);
    return pool_.make_lam(transpile(a.body, inner));
}

const lc_expr* transpiler::transpile_app(const aml_expr::app& a, const binding_env& env) {
    return pool_.make_app(transpile(a.func, env), transpile(a.arg, env));
}

const lc_expr* transpiler::transpile_list(const aml_expr::list& l, const binding_env& env) {
    std::vector<const lc_expr*> elems;
    elems.reserve(l.elems.size());
    for (const aml_expr* e : l.elems)
        elems.push_back(transpile(e, env));
    return l.church ? church_list(elems) : scott_list(elems);
}

const lc_expr* transpiler::transpile(const aml_expr* e, const binding_env& env) {
    return std::visit([&](const auto& alt) -> const lc_expr* {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, aml_expr::var>)
            return transpile_var(alt, env);
        else if constexpr (std::is_same_v<T, aml_expr::abs>)
            return transpile_abs(alt, env);
        else if constexpr (std::is_same_v<T, aml_expr::app>)
            return transpile_app(alt, env);
        else if constexpr (std::is_same_v<T, aml_expr::nat>)
            return transpile_nat(alt);
        else if constexpr (std::is_same_v<T, aml_expr::integer>)
            return transpile_integer(alt);
        else if constexpr (std::is_same_v<T, aml_expr::character>)
            return transpile_character(alt);
        else if constexpr (std::is_same_v<T, aml_expr::string>)
            return transpile_string(alt);
        else if constexpr (std::is_same_v<T, aml_expr::list>)
            return transpile_list(alt, env);
        else
            throw std::runtime_error("unsupported aml_expr variant");
    }, e->content);
}

const lc_expr* transpiler::transpile(const aml_expr* e) {
    binding_env env;
    return transpile(e, env);
}

const lc_expr* transpiler::transpile(const aml_expr* e,
                                     const std::map<std::string, const lc_expr*>& globals) {
    globals_ = &globals;
    binding_env env;
    return transpile(e, env);
}

transpiled_program transpiler::transpile_program(const aml_program& prog) {
    transpiled_program out;
    transpiler local(out.pool);
    local.globals_ = &out.globals;

    local.register_builtins(out.globals);
    for (const constructor_group& group : prog.groups)
        local.register_group(group, out.globals);

    for (const function_def& fn : prog.functions) {
        const lc_expr* body = local.transpile(fn.body, out.globals);
        out.functions.push_back({fn.name, body});
        out.globals[fn.name] = body;
    }

    return out;
}

bool lc_expr_eq(const lc_expr* a, const lc_expr* b) {
    if (a == b)
        return true;
    if (!a || !b)
        return false;

    return std::visit([&](const auto& av) -> bool {
        using T = std::decay_t<decltype(av)>;
        const auto* bv = std::get_if<T>(&b->content);
        if (!bv)
            return false;
        if constexpr (std::is_same_v<T, lc_expr::var>)
            return av.index == bv->index;
        else if constexpr (std::is_same_v<T, lc_expr::lam>)
            return lc_expr_eq(av.body, bv->body);
        else if constexpr (std::is_same_v<T, lc_expr::app>)
            return lc_expr_eq(av.func, bv->func) && lc_expr_eq(av.arg, bv->arg);
        else
            return false;
    }, a->content);
}
