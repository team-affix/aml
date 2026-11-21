// Swift on 10-bit parity: the proof that Swift finds the full parity function, the single leaf outputting the xor of
// all ten bits. It is the exact run recorded in README.md, with every setting fixed below; change nothing here, or it
// is no longer that run. It trains Swift on every row of the 10-bit truth table with xor, and, or and not for a fixed
// number of episodes, so it does the same work on any machine, and prints each new best model with how many xors its
// root holds, and every few thousand episodes the share of the episodes since the last report whose root holds 0, 1,
// ..., 9 and 10 or more xors. Episodes are deterministic; only the times depend on the machine.
//
// Build and run from the repository root:
//   make research/swift_parity
//   ./build/research/swift_parity

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <variant>
#include <vector>
#include <unistd.h>
#include "infrastructure/swift_runtime.hpp"
#include "infrastructure/schema.hpp"
#include "support/bool_and.hpp"
#include "support/bool_not.hpp"
#include "support/bool_or.hpp"
#include "support/bool_xor.hpp"

namespace {

// The run's settings.
const size_t k_bits = 10;                 // parity over this many input bits; the training set has 2^bits rows
const size_t k_max_applications = 9;      // synthesis applications per session
const double k_exploration_constant = 2;  // c, on ucb1's normalized [0, 1] scale
const double k_best_weight = 1;           // w: a node is read as (1 - w) * mean + w * best
const double k_fresh_weight = 0.75;       // in rollouts, the probability of a fresh operand over a consumed one
const double k_made_weight = 0.5;         // in rollouts, the probability of a made operand over a given one
const uint32_t k_seed = 2;
const size_t k_episodes = 25000;          // stop after this many episodes
const size_t k_report_episodes = 5000;    // episodes between progress lines
const size_t k_max_rss_mb = 4000;         // stop once resident memory passes this (checked every 1,000 episodes)
const size_t k_model_lines = 40;          // lines of the best model printed at the end
const size_t k_xor_buckets = 11;          // 0 to 9 xors, then 10 or more
const char* const k_primitive_names[] = {"xor", "and", "or", "not"};

volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) {
    stop_requested = 1;
}

// Counts of episodes by how many xors their root's feature holds (the last bucket is "or more"), and their summed cost.
struct root_tally {
    size_t episodes;
    double cost_sum;
    size_t by_xors[k_xor_buckets];
};

root_tally empty_tally() {
    return root_tally{.episodes = 0, .cost_sum = 0, .by_xors = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
}

double resident_mb() {
    long pages = 0;
    long resident = 0;
    FILE* statm = std::fopen("/proc/self/statm", "r");
    if (statm == nullptr)
        return 0;
    if (std::fscanf(statm, "%ld %ld", &pages, &resident) != 2)
        resident = 0;
    std::fclose(statm);
    return static_cast<double>(resident) * static_cast<double>(sysconf(_SC_PAGESIZE)) / 1048576.0;
}

size_t count_xors(const AML::feature& expression) {
    const AML::feature::app* application = std::get_if<AML::feature::app>(&expression.content);
    if (application == nullptr)
        return 0;
    size_t total = application->prim == 0 ? 1 : 0;
    for (const AML::feature& arg : application->args)
        total += count_xors(arg);
    return total;
}

// The xors in the feature the root matches on, or outputs if the root is a leaf.
size_t root_xors(const AML::model_node& model) {
    if (const AML::model_node::leaf* leaf = std::get_if<AML::model_node::leaf>(&model.content))
        return count_xors(leaf->output);
    const AML::model_node::match* match = std::get_if<AML::model_node::match>(&model.content);
    if (match == nullptr)
        return 0;
    return count_xors(match->scrutinee);
}

std::string show_feature(const AML::feature& expression, AML::ctor_id true_ctor) {
    if (const AML::feature::var* variable = std::get_if<AML::feature::var>(&expression.content))
        return "x" + std::to_string(variable->index);
    if (const AML::feature::constant* constant = std::get_if<AML::feature::constant>(&expression.content))
        return constant->literal.ctor == true_ctor ? "T" : "F";
    const AML::feature::app& application = std::get<AML::feature::app>(expression.content);
    std::string text = std::string(k_primitive_names[application.prim]) + "(";
    for (size_t index = 0; index < application.args.size(); ++index)
        text += (index == 0 ? "" : ", ") + show_feature(application.args[index], true_ctor);
    return text + ")";
}

std::string show_root(const AML::model_node& model, AML::ctor_id true_ctor) {
    if (const AML::model_node::match* match = std::get_if<AML::model_node::match>(&model.content))
        return "match " + show_feature(match->scrutinee, true_ctor);
    if (const AML::model_node::leaf* leaf = std::get_if<AML::model_node::leaf>(&model.content))
        return "leaf " + show_feature(leaf->output, true_ctor);
    return "empty";
}

void print_model(const AML::model_node& node, AML::ctor_id true_ctor, size_t depth, size_t& lines) {
    if (lines >= k_model_lines)
        return;
    const std::string indent(2 * depth + 2, ' ');
    ++lines;
    const AML::model_node::match* match = std::get_if<AML::model_node::match>(&node.content);
    if (match == nullptr) {
        std::printf("%s%s\n", indent.c_str(), show_root(node, true_ctor).c_str());
        return;
    }
    std::printf("%smatch %s\n", indent.c_str(), show_feature(match->scrutinee, true_ctor).c_str());
    for (size_t branch = 0; branch < match->branches.size(); ++branch) {
        if (lines >= k_model_lines)
            return;
        ++lines;
        std::printf("%s  %s ->\n", indent.c_str(), branch == true_ctor ? "T" : "F");
        print_model(match->branches[branch], true_ctor, depth + 2, lines);
    }
}

void record_episode(root_tally& tally, size_t xors, size_t cost) {
    ++tally.episodes;
    tally.cost_sum += static_cast<double>(cost);
    ++tally.by_xors[std::min(xors, k_xor_buckets - 1)];
}

void print_tally(const root_tally& tally) {
    if (tally.episodes == 0) {
        std::printf("no episodes");
        return;
    }
    const double episodes = static_cast<double>(tally.episodes);
    std::printf("mean cost %.0f | root xors", tally.cost_sum / episodes);
    for (size_t bucket = 0; bucket < k_xor_buckets; ++bucket) {
        if (tally.by_xors[bucket] == 0)
            continue;
        const bool last = bucket + 1 == k_xor_buckets;
        std::printf(" %zu%s:%.1f%%", bucket, last ? "+" : "",
                    100.0 * static_cast<double>(tally.by_xors[bucket]) / episodes);
    }
}

AML::bin parity_training(AML::type_id boolean, const AML::value& falsity, const AML::value& truth) {
    AML::bin training{.inputs = {}, .labels = AML::typed_column{.type = boolean, .cells = {}}};
    for (size_t bit = 0; bit < k_bits; ++bit)
        training.inputs.push_back(AML::typed_column{.type = boolean, .cells = {}});
    for (size_t row = 0; row < (size_t{1} << k_bits); ++row) {
        bool parity = false;
        for (size_t bit = 0; bit < k_bits; ++bit) {
            const bool set = ((row >> bit) & 1) == 1;
            parity = parity != set;
            training.inputs[bit].cells.push_back(set ? truth : falsity);
        }
        training.labels.cells.push_back(parity ? truth : falsity);
    }
    return training;
}

double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

}

int main() {
    AML::schema types;
    const AML::type_id boolean = types.declare_type();
    const AML::ctor_id false_ctor = types.declare_ctor(boolean, {});
    const AML::ctor_id true_ctor = types.declare_ctor(boolean, {});
    const AML::value falsity{.ctor = false_ctor, .args = {}};
    const AML::value truth{.ctor = true_ctor, .args = {}};
    const AML::primitive_type binary{.arg_types = {boolean, boolean}, .result_type = boolean};
    const AML::primitive_type unary{.arg_types = {boolean}, .result_type = boolean};
    // In the order of k_primitive_names: xor is primitive 0.
    const std::vector<AML::typed_primitive> primitives{
        {.type = binary, .apply = bool_xor(false_ctor, true_ctor)},
        {.type = binary, .apply = bool_and(false_ctor, true_ctor)},
        {.type = binary, .apply = bool_or(false_ctor, true_ctor)},
        {.type = unary, .apply = bool_not(false_ctor, true_ctor)}};
    const std::vector<AML::typed_constant> constants{
        {.type = boolean, .literal = falsity},
        {.type = boolean, .literal = truth}};
    const AML::bin training = parity_training(boolean, falsity, truth);

    AML::swift_runtime runtime(types, primitives, constants, training,
                               AML::swift_options{.max_applications = k_max_applications,
                                                  .exploration_constant = k_exploration_constant,
                                                  .best_weight = k_best_weight, .fresh_weight = k_fresh_weight,
                                                  .made_weight = k_made_weight, .seed = k_seed});
    std::printf("Swift on %zu-bit parity (%zu rows), xor/and/or/not: max applications %zu, c %.3g, best weight %.3g, "
                "fresh weight %.3g, made weight %.3g, seed %u, %zu episodes\n", k_bits, training.labels.cells.size(),
                k_max_applications, k_exploration_constant, k_best_weight, k_fresh_weight, k_made_weight, k_seed,
                k_episodes);
    std::fflush(stdout);
    std::signal(SIGINT, request_stop);

    const auto start = std::chrono::steady_clock::now();
    root_tally whole_run = empty_tally();
    root_tally window = empty_tally();
    size_t episode = 0;
    size_t best_cost = SIZE_MAX;
    const char* stop_reason = "Ctrl-C";
    while (stop_requested == 0) {
        const AML::episode_outcome outcome = runtime.next();
        ++episode;
        const size_t xors = root_xors(outcome.model);
        record_episode(whole_run, xors, outcome.cost);
        record_episode(window, xors, outcome.cost);

        const double elapsed = seconds_since(start);
        if (runtime.best_cost() < best_cost) {
            best_cost = runtime.best_cost();
            const AML::model_node& best = runtime.best_model().value();
            std::printf("  new best %zu at %.1fs (episode %zu) [%zu xors]: %s\n", best_cost, elapsed, episode,
                        root_xors(best), show_root(best, true_ctor).c_str());
        }
        const bool report = episode % k_report_episodes == 0;
        if (report) {
            std::printf("[episode %6zu, %6.1fs] %.0f/s | rss %.0f MB | best %zu | last %zu episodes: ", episode,
                        elapsed, static_cast<double>(episode) / elapsed, resident_mb(), best_cost, k_report_episodes);
            print_tally(window);
            std::printf("\n");
            window = empty_tally();
        }
        std::fflush(stdout);
        if (episode >= k_episodes) {
            stop_reason = "episode limit";
            break;
        }
        const bool check_memory = episode % 1000 == 0;
        if (check_memory && resident_mb() > static_cast<double>(k_max_rss_mb)) {
            stop_reason = "memory limit";
            break;
        }
    }

    std::printf("\nStopped (%s) after %.1fs: %zu episodes, rss %.0f MB\n", stop_reason, seconds_since(start), episode,
                resident_mb());
    std::printf("Whole run: ");
    print_tally(whole_run);
    std::printf("\n");
    if (!runtime.best_model().has_value())
        return 0;
    std::printf("Best model, cost %zu:\n", runtime.best_cost());
    size_t lines = 0;
    print_model(runtime.best_model().value(), true_ctor, 0, lines);
    return 0;
}
