// Swift on a reused product: the proof that Swift finds f(x, y, z) = (x * y + z)^2 - x * y from its values, as the
// formula itself, in which the product x * y appears twice. It is the exact run recorded in README.md, with every
// setting fixed below; change nothing here, or it is no longer that run. Numbers are binary naturals, a user type with
// the constructors zero, twice(n) (2n) and twice_plus_one(n) (2n + 1). Swift trains on every (x, y, z) with each input
// from 0 to 3, with nine primitives (add, sub truncated at zero, mul, square, succ, pred truncated at zero, double,
// half and muladd), for a fixed number of episodes, so it does the same work on any machine. At the end it checks that
// the best model is exactly the leaf sub(square(add(z, mul(x, y))), mul(x, y)), right on every training row and on
// 200 unseen rows with inputs up to 30, prints PROOF PASSED or PROOF FAILED, and exits with 0 or 1.
//
// Build and run from the repository root:
//   make research/swift_reused_product
//   ./build/research/swift_reused_product

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include "infrastructure/schema.hpp"
#include "infrastructure/swift_runtime.hpp"

namespace {

// The run's settings.
const size_t k_max_input = 3;              // training inputs: every (x, y, z) with each from 0 to this
const size_t k_unseen_rows = 200;          // unseen inputs: this many other (x, y, z), drawn as below
const size_t k_max_unseen_input = 30;      // each unseen input is drawn from 0 to this
const uint32_t k_unseen_seed = 54321;      // the draw of the unseen inputs
const size_t k_max_applications = 9;       // synthesis applications per session
const double k_exploration_constant = 4;   // c, on ucb1's normalized [0, 1] scale
const double k_best_weight = 1;            // w: a node is read as (1 - w) * mean + w * best
const double k_fresh_weight = 0.75;        // in rollouts, the probability of a fresh operand over a consumed one
const double k_made_weight = 0.5;          // in rollouts, the probability of a made operand over a given one
const uint32_t k_seed = 1;
const size_t k_episodes = 600000;          // stop after this many episodes
const size_t k_report_episodes = 100000;   // episodes between progress lines
const size_t k_largest = size_t{1} << 40;  // every primitive's result is capped here

// What the proof must derive: the formula as one leaf, sub(square(add(z, mul(x, y))), mul(x, y)), cost 10.
const size_t k_expected_cost = 10;
const char* const k_primitive_names[] = {"add", "sub", "mul", "square", "succ", "pred", "double", "half", "muladd"};
const AML::prim_id k_add = 0;
const AML::prim_id k_sub = 1;
const AML::prim_id k_mul = 2;
const AML::prim_id k_square = 3;

size_t target(size_t x, size_t y, size_t z) {
    const size_t product = x * y;
    return (product + z) * (product + z) - product;
}

AML::feature expected_formula() {
    const AML::feature x{AML::feature::var{.index = 0}};
    const AML::feature y{AML::feature::var{.index = 1}};
    const AML::feature z{AML::feature::var{.index = 2}};
    const AML::feature product{AML::feature::app{.prim = k_mul, .args = {x, y}}};
    const AML::feature sum{AML::feature::app{.prim = k_add, .args = {z, product}}};
    const AML::feature square{AML::feature::app{.prim = k_square, .args = {sum}}};
    return AML::feature{AML::feature::app{.prim = k_sub, .args = {square, product}}};
}

// The binary naturals' constructors, and conversion between their values and numbers.
struct binary_naturals {
    AML::ctor_id zero;
    AML::ctor_id twice;
    AML::ctor_id twice_plus_one;
};

size_t number_of(const AML::value& term, const binary_naturals& naturals) {
    if (term.ctor == naturals.zero)
        return 0;
    const size_t low_bit = term.ctor == naturals.twice_plus_one ? 1 : 0;
    return 2 * number_of(term.args[0], naturals) + low_bit;
}

AML::value value_of(size_t number, const binary_naturals& naturals) {
    if (number == 0)
        return AML::value{.ctor = naturals.zero, .args = {}};
    const bool odd = number % 2 == 1;
    return AML::value{.ctor = odd ? naturals.twice_plus_one : naturals.twice, .args = {value_of(number / 2, naturals)}};
}

AML::value capped_value_of(unsigned __int128 number, const binary_naturals& naturals) {
    const bool too_large = number > k_largest;
    return value_of(too_large ? k_largest : static_cast<size_t>(number), naturals);
}

struct natural_add {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const unsigned __int128 sum = number_of(*args[0], naturals);
        return capped_value_of(sum + number_of(*args[1], naturals), naturals);
    }
};

struct natural_sub {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const size_t left = number_of(*args[0], naturals);
        const size_t right = number_of(*args[1], naturals);
        return value_of(left > right ? left - right : 0, naturals);
    }
};

struct natural_mul {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const unsigned __int128 product = number_of(*args[0], naturals);
        return capped_value_of(product * number_of(*args[1], naturals), naturals);
    }
};

struct natural_square {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const unsigned __int128 number = number_of(*args[0], naturals);
        return capped_value_of(number * number, naturals);
    }
};

struct natural_succ {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const unsigned __int128 number = number_of(*args[0], naturals);
        return capped_value_of(number + 1, naturals);
    }
};

struct natural_pred {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const size_t number = number_of(*args[0], naturals);
        return value_of(number > 0 ? number - 1 : 0, naturals);
    }
};

struct natural_double {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const unsigned __int128 number = number_of(*args[0], naturals);
        return capped_value_of(number * 2, naturals);
    }
};

struct natural_half {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        return value_of(number_of(*args[0], naturals) / 2, naturals);
    }
};

struct natural_muladd {
    binary_naturals naturals;
    AML::value operator()(const std::vector<const AML::value*>& args) const {
        const unsigned __int128 product = number_of(*args[0], naturals);
        return capped_value_of(product * number_of(*args[1], naturals) + number_of(*args[2], naturals), naturals);
    }
};

std::string show_feature(const AML::feature& expression, const binary_naturals& naturals) {
    if (const AML::feature::var* variable = std::get_if<AML::feature::var>(&expression.content)) {
        const char* const names[] = {"x", "y", "z"};
        return variable->index < 3 ? names[variable->index] : "v" + std::to_string(variable->index);
    }
    if (const AML::feature::constant* constant = std::get_if<AML::feature::constant>(&expression.content))
        return std::to_string(number_of(constant->literal, naturals));
    const AML::feature::app& application = std::get<AML::feature::app>(expression.content);
    std::string text = std::string(k_primitive_names[application.prim]) + "(";
    for (size_t index = 0; index < application.args.size(); ++index)
        text += (index == 0 ? "" : ", ") + show_feature(application.args[index], naturals);
    return text + ")";
}

std::string show_root(const AML::model_node& model, const binary_naturals& naturals) {
    if (const AML::model_node::match* match = std::get_if<AML::model_node::match>(&model.content))
        return "match " + show_feature(match->scrutinee, naturals);
    if (const AML::model_node::leaf* leaf = std::get_if<AML::model_node::leaf>(&model.content))
        return "leaf " + show_feature(leaf->output, naturals);
    return "empty";
}

bool infers_target(const AML::swift_runtime& runtime, size_t x, size_t y, size_t z, const binary_naturals& naturals) {
    const std::optional<AML::value> inferred =
        runtime.infer({value_of(x, naturals), value_of(y, naturals), value_of(z, naturals)});
    return inferred.has_value() && number_of(inferred.value(), naturals) == target(x, y, z);
}

double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

}

int main() {
    AML::schema types;
    const AML::type_id natural = types.declare_type();
    const AML::ctor_id zero = types.declare_ctor(natural, {});
    const AML::ctor_id twice = types.declare_ctor(natural, {natural});
    const AML::ctor_id twice_plus_one = types.declare_ctor(natural, {natural});
    const binary_naturals naturals{.zero = zero, .twice = twice, .twice_plus_one = twice_plus_one};
    const AML::primitive_type unary{.arg_types = {natural}, .result_type = natural};
    const AML::primitive_type binary{.arg_types = {natural, natural}, .result_type = natural};
    const AML::primitive_type ternary{.arg_types = {natural, natural, natural}, .result_type = natural};
    // In the order of k_primitive_names.
    const std::vector<AML::typed_primitive> primitives{
        {.type = binary, .apply = natural_add{.naturals = naturals}},
        {.type = binary, .apply = natural_sub{.naturals = naturals}},
        {.type = binary, .apply = natural_mul{.naturals = naturals}},
        {.type = unary, .apply = natural_square{.naturals = naturals}},
        {.type = unary, .apply = natural_succ{.naturals = naturals}},
        {.type = unary, .apply = natural_pred{.naturals = naturals}},
        {.type = unary, .apply = natural_double{.naturals = naturals}},
        {.type = unary, .apply = natural_half{.naturals = naturals}},
        {.type = ternary, .apply = natural_muladd{.naturals = naturals}}};
    const std::vector<AML::typed_constant> constants{
        {.type = natural, .literal = value_of(0, naturals)},
        {.type = natural, .literal = value_of(1, naturals)}};

    AML::bin training{.inputs = {}, .labels = AML::typed_column{.type = natural, .cells = {}}};
    for (size_t input = 0; input < 3; ++input)
        training.inputs.push_back(AML::typed_column{.type = natural, .cells = {}});
    for (size_t x = 0; x <= k_max_input; ++x) {
        for (size_t y = 0; y <= k_max_input; ++y) {
            for (size_t z = 0; z <= k_max_input; ++z) {
                training.inputs[0].cells.push_back(value_of(x, naturals));
                training.inputs[1].cells.push_back(value_of(y, naturals));
                training.inputs[2].cells.push_back(value_of(z, naturals));
                training.labels.cells.push_back(value_of(target(x, y, z), naturals));
            }
        }
    }

    AML::swift_runtime runtime(types, primitives, constants, training,
                               AML::swift_options{.max_applications = k_max_applications,
                                                  .exploration_constant = k_exploration_constant,
                                                  .best_weight = k_best_weight, .fresh_weight = k_fresh_weight,
                                                  .made_weight = k_made_weight, .seed = k_seed});
    std::printf("Swift on (x * y + z)^2 - x * y, inputs 0 to %zu (%zu rows), nine primitives: max applications %zu, "
                "c %.3g, best weight %.3g, fresh weight %.3g, made weight %.3g, seed %u, %zu episodes\n", k_max_input,
                training.labels.cells.size(), k_max_applications, k_exploration_constant, k_best_weight,
                k_fresh_weight, k_made_weight, k_seed, k_episodes);
    std::fflush(stdout);

    const auto start = std::chrono::steady_clock::now();
    size_t best_cost = SIZE_MAX;
    double cost_sum = 0;
    for (size_t episode = 1; episode <= k_episodes; ++episode) {
        const AML::episode_outcome outcome = runtime.next();
        cost_sum += static_cast<double>(outcome.cost);
        if (runtime.best_cost() < best_cost) {
            best_cost = runtime.best_cost();
            std::printf("  new best %zu at %.1fs (episode %zu): %s\n", best_cost, seconds_since(start), episode,
                        show_root(runtime.best_model().value(), naturals).c_str());
        }
        const bool report = episode % k_report_episodes == 0;
        if (report) {
            std::printf("[episode %6zu, %6.1fs] best %zu | last %zu episodes: mean cost %.0f\n", episode,
                        seconds_since(start), best_cost, k_report_episodes,
                        cost_sum / static_cast<double>(k_report_episodes));
            cost_sum = 0;
        }
        std::fflush(stdout);
    }

    const AML::model_node& best = runtime.best_model().value();
    std::printf("\nBest model, cost %zu: %s\n", runtime.best_cost(), show_root(best, naturals).c_str());

    // The proof's claim: the best model is exactly the leaf sub(square(add(z, mul(x, y))), mul(x, y)) (cost 10), right
    // on every training row and on k_unseen_rows other rows drawn with inputs up to k_max_unseen_input.
    std::set<std::tuple<size_t, size_t, size_t>> used;
    size_t wrong_training = 0;
    for (size_t x = 0; x <= k_max_input; ++x) {
        for (size_t y = 0; y <= k_max_input; ++y) {
            for (size_t z = 0; z <= k_max_input; ++z) {
                used.insert({x, y, z});
                wrong_training += infers_target(runtime, x, y, z, naturals) ? 0 : 1;
            }
        }
    }
    std::mt19937 unseen_random(k_unseen_seed);
    std::uniform_int_distribution<size_t> unseen_input(0, k_max_unseen_input);
    size_t unseen = 0;
    size_t wrong_unseen = 0;
    while (unseen < k_unseen_rows) {
        const size_t x = unseen_input(unseen_random);
        const size_t y = unseen_input(unseen_random);
        const size_t z = unseen_input(unseen_random);
        if (!used.insert({x, y, z}).second)
            continue;
        ++unseen;
        wrong_unseen += infers_target(runtime, x, y, z, naturals) ? 0 : 1;
    }
    const AML::model_node::leaf* leaf = std::get_if<AML::model_node::leaf>(&best.content);
    const bool exact = leaf != nullptr && leaf->output == expected_formula();
    const bool passed = runtime.best_cost() == k_expected_cost && exact && wrong_training == 0 && wrong_unseen == 0;
    std::printf("PROOF %s: best cost %zu (expected %zu), %s, %zu of %zu training rows wrong, %zu of %zu unseen rows "
                "wrong\n", passed ? "PASSED" : "FAILED", runtime.best_cost(), k_expected_cost,
                exact ? "exactly the expected leaf" : "not the expected leaf", wrong_training,
                training.labels.cells.size(), wrong_unseen, unseen);
    return passed ? 0 : 1;
}
