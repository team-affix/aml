#ifndef SCOTT_ENCODER_HPP
#define SCOTT_ENCODER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/builtin_constructors.hpp"
#include "infrastructure/name_env.hpp"
#include "value_objects/constructor_group.hpp"
#include "value_objects/lc_expr.hpp"

struct encoded_binding {
    std::string    name;
    const lc_expr* definition;
};

template<typename IMakeLcVar, typename IMakeLcLam, typename IMakeLcApp>
struct scott_encoder {
    scott_encoder(IMakeLcVar& make_var, IMakeLcLam& make_lam, IMakeLcApp& make_app);

    std::vector<encoded_binding> encode_builtins();
    std::vector<encoded_binding> encode_constructor_group(const constructor_group& group);

    const lc_expr* scott_nat(uint64_t value, const global_env& global);
    const lc_expr* scott_list(const std::vector<const lc_expr*>& elems, const global_env& global);
    const lc_expr* scott_integer(int64_t value, const global_env& global,
                                 const local_binding_env& local);
    const lc_expr* scott_string(const std::string& value, const global_env& global,
                                  const local_binding_env& local);

    const lc_expr* church_nat(uint64_t value);
    const lc_expr* church_list(const std::vector<const lc_expr*>& elems);

private:
    IMakeLcVar& make_var_;
    IMakeLcLam& make_lam_;
    IMakeLcApp& make_app_;

    const lc_expr* scott_constructor(uint32_t index, uint32_t group_size, uint32_t arity);
    const lc_expr* global_var(const global_env& global, const std::string& name,
                              uint32_t local_depth);
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

template<typename IV, typename IL, typename IA>
scott_encoder<IV, IL, IA>::scott_encoder(IV& make_var, IL& make_lam, IA& make_app)
    : make_var_(make_var), make_lam_(make_lam), make_app_(make_app) {}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::scott_constructor(uint32_t index,
                                                              uint32_t group_size,
                                                              uint32_t arity) {
    if (index >= group_size)
        throw std::runtime_error("constructor index out of range");
    if (group_size == 0)
        throw std::runtime_error("empty constructor group");

    const lc_expr* body = make_var_.make_var(group_size - 1 - index);
    for (uint32_t i = 0; i < arity; ++i) {
        uint32_t arg_idx = group_size + arity - 1 - i;
        body = make_app_.make_app(body, make_var_.make_var(arg_idx));
    }
    for (uint32_t i = 0; i < group_size; ++i)
        body = make_lam_.make_lam(body);
    for (uint32_t i = 0; i < arity; ++i)
        body = make_lam_.make_lam(body);
    return body;
}

template<typename IV, typename IL, typename IA>
std::vector<encoded_binding> scott_encoder<IV, IL, IA>::encode_builtins() {
    std::vector<encoded_binding> out;
    for (const constructor_group& group :
         {builtin_bool_group, builtin_list_group, builtin_int_group}) {
        auto encoded = encode_constructor_group(group);
        out.insert(out.end(), encoded.begin(), encoded.end());
    }
    return out;
}

template<typename IV, typename IL, typename IA>
std::vector<encoded_binding> scott_encoder<IV, IL, IA>::encode_constructor_group(
    const constructor_group& group) {
    const uint32_t n = static_cast<uint32_t>(group.constructors.size());
    std::vector<encoded_binding> out;
    out.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        const auto& decl = group.constructors[i];
        out.push_back({decl.name, scott_constructor(i, n, decl.arity)});
    }
    return out;
}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::global_var(const global_env& global,
                                                      const std::string& name,
                                                      uint32_t local_depth) {
    auto k = global.lookup_global(name);
    if (!k)
        throw std::runtime_error("unbound global name: " + name);
    return make_var_.make_var(local_depth + *k);
}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::scott_nat(uint64_t value, const global_env& global) {
    const lc_expr* list = global_var(global, "nil", 0);
    if (value == 0)
        return list;

    std::vector<bool> bits;
    for (uint64_t v = value; v > 0; v >>= 1)
        bits.push_back((v & 1) != 0);
    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        const lc_expr* bit = *it ? global_var(global, "true", 0)
                                 : global_var(global, "false", 0);
        list = make_app_.make_app(
            make_app_.make_app(global_var(global, "cons", 0), bit), list);
    }
    return list;
}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::scott_list(const std::vector<const lc_expr*>& elems,
                                                       const global_env& global) {
    const lc_expr* list = global_var(global, "nil", 0);
    for (auto it = elems.rbegin(); it != elems.rend(); ++it)
        list = make_app_.make_app(
            make_app_.make_app(global_var(global, "cons", 0), *it), list);
    return list;
}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::scott_integer(int64_t value, const global_env& global,
                                                         const local_binding_env& local) {
    const uint32_t depth = local.depth();
    if (value > 0)
        return make_app_.make_app(global_var(global, "pos", depth),
                                  scott_nat(static_cast<uint64_t>(value), global));
    if (value < 0)
        return make_app_.make_app(global_var(global, "negsuc", depth),
                                  scott_nat(static_cast<uint64_t>(-value - 1), global));
    return make_app_.make_app(global_var(global, "pos", depth), scott_nat(0, global));
}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::scott_string(const std::string& value,
                                                        const global_env& global,
                                                        const local_binding_env& local) {
    std::vector<const lc_expr*> elems;
    elems.reserve(value.size());
    for (unsigned char ch : value)
        elems.push_back(scott_integer(static_cast<int64_t>(ch), global, local));
    return scott_list(elems, global);
}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::church_nat(uint64_t value) {
    const lc_expr* body = make_var_.make_var(0);
    for (uint64_t i = 0; i < value; ++i)
        body = make_app_.make_app(make_var_.make_var(1), body);
    return make_lam_.make_lam(make_lam_.make_lam(body));
}

template<typename IV, typename IL, typename IA>
const lc_expr* scott_encoder<IV, IL, IA>::church_list(
    const std::vector<const lc_expr*>& elems) {
    const lc_expr* body = make_var_.make_var(0);
    for (auto it = elems.rbegin(); it != elems.rend(); ++it)
        body = make_app_.make_app(make_app_.make_app(make_var_.make_var(1), *it), body);
    return make_lam_.make_lam(make_lam_.make_lam(body));
}

#endif
