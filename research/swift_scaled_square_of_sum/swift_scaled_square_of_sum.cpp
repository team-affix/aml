// Swift on the scaled square of a sum: the proof that Swift finds f(x, y, z) = (x + y)^2 * z from its values, as the
// formula itself.
// It is the exact run recorded in README.md, with every setting fixed below; change nothing here, or it is no longer
// that run. Numbers are binary naturals, a user type with the constructors zero, twice(n) (2n) and twice_plus_one(n)
// (2n + 1), so a number takes about log2 of its value in nodes. Swift trains on every (x, y, z) with each input from 0
// to 3, with add, sub (truncated at zero), mul and square, for a fixed number of episodes, so it does the same work on
// any machine. It prints each new best model and every few thousand episodes a progress line. At the end it checks
// that the best model is the formula, right on every training row and on every unseen row with inputs up to 5, prints
// PROOF PASSED or PROOF FAILED, and exits with 0 or 1.
//
// Build and run from the repository root:
//   make research/swift_scaled_square_of_sum
//   ./build/research/swift_scaled_square_of_sum

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "infrastructure/schema.hpp"
#include "infrastructure/swift_runtime.hpp"

namespace {

// The run's settings.
const size_t k_max_input = 3;             // training inputs: every (x, y, z) with each from 0 to this
const size_t k_max_unseen_input = 5;      // unseen inputs: every other (x, y, z) with each from 0 to this
const size_t k_max_applications = 9;      // synthesis applications per session
const double k_exploration_constant = 2;  // c, on ucb1's normalized [0, 1] scale
const double k_best_weight = 1;           // w: a node is read as (1 - w) * mean + w * best
const double k_fresh_weight = 0.75;       // in rollouts, the probability of a fresh operand over a consumed one
const double k_made_weight = 0.5;         // in rollouts, the probability of a made operand over a given one
const uint32_t k_seed = 1;
const size_t k_episodes = 10000;          // stop after this many episodes
const size_t k_report_episodes = 2000;    // episodes between progress lines
const size_t k_largest = size_t{1} << 40; // every primitive's result is capped here

// What the proof must derive: the formula as one leaf, mul(square(add(x, y)), z), cost 6.
const size_t k_expected_cost = 6;
const char* const k_primitive_names[] = {"add", "sub", "mul", "square"};

size_t target(size_t x, size_t y, size_t z) {
    return (x + y) * (x + y) * z;
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
    const AML::primitive_type binary{.arg_types = {natural, natural}, .result_type = natural};
    const AML::primitive_type unary{.arg_types = {natural}, .result_type = natural};
    // In the order of k_primitive_names.
    const std::vector<AML::typed_primitive> primitives{
        {.type = binary, .apply = natural_add{.naturals = naturals}},
        {.type = binary, .apply = natural_sub{.naturals = naturals}},
        {.type = binary, .apply = natural_mul{.naturals = naturals}},
        {.type = unary, .apply = natural_square{.naturals = naturals}}};
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
    std::printf("Swift on (x + y)^2 * z, inputs 0 to %zu (%zu rows), add/sub/mul/square: max applications %zu, c %.3g, "
                "best weight %.3g, fresh weight %.3g, made weight %.3g, seed %u, %zu episodes\n", k_max_input,
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

    // The proof's claim: the best model is the formula as one leaf (cost 6), right on every training row and on every
    // unseen row up to k_max_unseen_input.
    size_t wrong_training = 0;
    size_t wrong_unseen = 0;
    size_t unseen = 0;
    for (size_t x = 0; x <= k_max_unseen_input; ++x) {
        for (size_t y = 0; y <= k_max_unseen_input; ++y) {
            for (size_t z = 0; z <= k_max_unseen_input; ++z) {
                const std::optional<AML::value> inferred =
                    runtime.infer({value_of(x, naturals), value_of(y, naturals), value_of(z, naturals)});
                const bool right = inferred.has_value() && number_of(inferred.value(), naturals) == target(x, y, z);
                const bool seen = x <= k_max_input && y <= k_max_input && z <= k_max_input;
                unseen += seen ? 0 : 1;
                wrong_training += seen && !right ? 1 : 0;
                wrong_unseen += !seen && !right ? 1 : 0;
            }
        }
    }
    const bool is_leaf = std::holds_alternative<AML::model_node::leaf>(best.content);
    const bool passed = runtime.best_cost() == k_expected_cost && is_leaf && wrong_training == 0 && wrong_unseen == 0;
    std::printf("PROOF %s: best cost %zu (expected %zu), %s, %zu of %zu training rows wrong, %zu of %zu unseen rows "
                "wrong\n", passed ? "PASSED" : "FAILED", runtime.best_cost(), k_expected_cost,
                is_leaf ? "a leaf" : "not a leaf", wrong_training, training.labels.cells.size(), wrong_unseen, unseen);
    return passed ? 0 : 1;
}
