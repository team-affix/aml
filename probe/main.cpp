// Probe: a developer's entry point for watching a runtime run, outside the core library. Swap this file's example for
// any other. The current example trains Falcon on n-bit parity (every row of the truth table) and prints its progress
// until a time limit, a memory limit or Ctrl-C, then a summary with the best model found. See probe/README.md.

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <variant>
#include <vector>
#include <unistd.h>
#include "infrastructure/falcon_manifest.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/value_pool.hpp"
#include "probe/bool_xor.hpp"
#include "support/bool_and.hpp"
#include "support/bool_not.hpp"
#include "support/bool_or.hpp"

namespace {

const AML::prim_id k_xor = 0;
const char* const k_primitive_names[] = {"xor", "and", "or", "not"};
const size_t k_xor_buckets = 6;

// Settings for this example: edit them here.
const size_t k_bits = 10;                // parity over this many input bits; the training set has 2^bits rows
const size_t k_max_applications = 9;     // synthesis applications per session
const double k_exploration_constant = 4; // on ucb1's normalized [0, 1] scale; above 1 favours exploration
const uint32_t k_seed = 1;
const size_t k_commit_visits = 52000;    // frontier visits before committing along the best path; 0 never commits
const double k_seconds = 0;              // stop after this many seconds; 0 runs until Ctrl-C
const double k_report_seconds = 10;      // seconds between progress lines
const size_t k_max_rss_mb = 4000;        // stop once resident memory passes this (checked every 1,000 episodes)
const size_t k_model_lines = 40;         // lines of the best model printed at the end

volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) {
    stop_requested = 1;
}

// Counts of episodes by how many xors the feature their root matches on holds (the last bucket is "or more"), and
// their summed cost.
struct root_tally {
    size_t episodes;
    double cost_sum;
    size_t by_xors[k_xor_buckets];
};

root_tally empty_tally() {
    return root_tally{.episodes = 0, .cost_sum = 0, .by_xors = {0, 0, 0, 0, 0, 0}};
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
    size_t total = application->prim == k_xor ? 1 : 0;
    for (const AML::feature& arg : application->args)
        total += count_xors(arg);
    return total;
}

std::string show_feature(const AML::feature& expression, size_t true_tag) {
    if (const AML::feature::var* variable = std::get_if<AML::feature::var>(&expression.content))
        return "x" + std::to_string(variable->index);
    if (const AML::feature::constant* constant = std::get_if<AML::feature::constant>(&expression.content))
        return constant->literal->tag == true_tag ? "T" : "F";
    const AML::feature::app& application = std::get<AML::feature::app>(expression.content);
    std::string text = std::string(k_primitive_names[application.prim]) + "(";
    for (size_t index = 0; index < application.args.size(); ++index)
        text += (index == 0 ? "" : ", ") + show_feature(application.args[index], true_tag);
    return text + ")";
}

std::string show_root(const AML::model_node& model, size_t true_tag) {
    if (const AML::model_node::match* match = std::get_if<AML::model_node::match>(&model.content))
        return "match " + show_feature(match->scrutinee, true_tag);
    if (const AML::model_node::leaf* leaf = std::get_if<AML::model_node::leaf>(&model.content))
        return "leaf " + show_feature(leaf->output, true_tag);
    return "empty";
}

size_t root_xors(const AML::model_node& model) {
    const AML::model_node::match* match = std::get_if<AML::model_node::match>(&model.content);
    if (match == nullptr)
        return 0;
    return count_xors(match->scrutinee);
}

void print_model(const AML::model_node& node, size_t true_tag, size_t depth, size_t max_lines, size_t& lines) {
    if (lines >= max_lines)
        return;
    const std::string indent(2 * depth + 2, ' ');
    ++lines;
    const AML::model_node::match* match = std::get_if<AML::model_node::match>(&node.content);
    if (match == nullptr) {
        std::printf("%s%s\n", indent.c_str(), show_root(node, true_tag).c_str());
        return;
    }
    std::printf("%smatch %s\n", indent.c_str(), show_feature(match->scrutinee, true_tag).c_str());
    for (size_t branch = 0; branch < match->branches.size(); ++branch) {
        if (lines >= max_lines)
            return;
        ++lines;
        std::printf("%s  %s ->\n", indent.c_str(), branch == true_tag ? "T" : "F");
        print_model(match->branches[branch], true_tag, depth + 2, max_lines, lines);
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
    for (size_t bucket = 0; bucket < k_xor_buckets; ++bucket)
        std::printf(" %zu%s:%.1f%%", bucket, bucket + 1 == k_xor_buckets ? "+" : "",
                    100.0 * static_cast<double>(tally.by_xors[bucket]) / episodes);
}

AML::bin parity_training(AML::type_id boolean, const AML::value* falsity, const AML::value* truth, size_t bits) {
    AML::bin training{.inputs = {}, .labels = AML::typed_column{.type = boolean, .cells = {}}};
    for (size_t bit = 0; bit < bits; ++bit)
        training.inputs.push_back(AML::typed_column{.type = boolean, .cells = {}});
    for (size_t row = 0; row < (size_t{1} << bits); ++row) {
        bool parity = false;
        for (size_t bit = 0; bit < bits; ++bit) {
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
    const size_t commit_visits = k_commit_visits == 0 ? std::numeric_limits<size_t>::max() : k_commit_visits;

    AML::schema types;
    const AML::type_id boolean = types.declare_type();
    const size_t false_tag = types.declare_ctor(boolean, {});
    const size_t true_tag = types.declare_ctor(boolean, {});

    AML::value_pool values;
    const AML::value* falsity = values.make(false_tag, {});
    const AML::value* truth = values.make(true_tag, {});
    const AML::primitive_type binary{.arg_types = {boolean, boolean}, .result_type = boolean};
    const std::vector<AML::typed_primitive> primitives{
        {.type = binary, .apply = bool_xor<AML::value_pool>(values, false_tag, true_tag)},
        {.type = binary, .apply = bool_and<AML::value_pool>(values, false_tag, true_tag)},
        {.type = binary, .apply = bool_or<AML::value_pool>(values, false_tag, true_tag)},
        {.type = {.arg_types = {boolean}, .result_type = boolean},
         .apply = bool_not<AML::value_pool>(values, false_tag, true_tag)}};
    const std::vector<AML::typed_constant> constants{
        {.type = boolean, .literal = falsity},
        {.type = boolean, .literal = truth}};
    const AML::bin training = parity_training(boolean, falsity, truth, k_bits);

    // The manifest directly, not falcon_runtime: the probe also watches the commitments, which no runtime exposes.
    AML::falcon_manifest manifest(types, values, primitives,
                                  AML::falcon_options{.max_applications = k_max_applications,
                                                      .exploration_constant = k_exploration_constant,
                                                      .seed = k_seed,
                                                      .commit_visits = commit_visits});
    manifest.constant_seeder_.seed(constants);

    std::signal(SIGINT, request_stop);
    std::printf("Falcon on %zu-bit parity (%zu rows): max applications %zu, c %.3g, seed %u, commit visits %s\n",
                k_bits, training.labels.cells.size(), k_max_applications,
                k_exploration_constant, k_seed,
                k_commit_visits == 0 ? "never" : std::to_string(k_commit_visits).c_str());
    std::printf("Primitives: xor, and, or, not. Constants: F, T. Ctrl-C stops and prints a summary.\n\n");
    std::fflush(stdout);

    const auto start = std::chrono::steady_clock::now();
    root_tally whole_run = empty_tally();
    root_tally window = empty_tally();
    size_t episode = 0;
    size_t best_cost = std::numeric_limits<size_t>::max();
    size_t commitments = 0;
    double next_report = k_report_seconds;
    const char* stop_reason = "Ctrl-C";
    while (stop_requested == 0) {
        const AML::episode_outcome outcome = manifest.mcts_episode_.run(training);
        ++episode;
        const size_t xors = root_xors(outcome.model);
        record_episode(whole_run, xors, outcome.cost);
        record_episode(window, xors, outcome.cost);

        const double elapsed = seconds_since(start);
        if (manifest.best_model_tracker_.best_cost() < best_cost) {
            best_cost = manifest.best_model_tracker_.best_cost();
            std::printf("  new best %zu at %.1fs (episode %zu): %s\n", best_cost, elapsed, episode,
                        show_root(manifest.best_model_tracker_.best_model().value(), true_tag).c_str());
        }
        if (manifest.commitments_.commitment_count() != commitments) {
            commitments = manifest.commitments_.commitment_count();
            std::printf("  commitment %zu at %.1fs (episode %zu)\n", commitments, elapsed, episode);
        }
        if (elapsed >= next_report) {
            next_report += k_report_seconds;
            std::printf("[%6.0fs] episodes %zu (%.0f/s) | rss %.0f MB | best %zu | commitments %zu | last %.0fs: ",
                        elapsed, episode, static_cast<double>(episode) / elapsed, resident_mb(), best_cost,
                        commitments, k_report_seconds);
            print_tally(window);
            std::printf("\n");
            window = empty_tally();
        }
        std::fflush(stdout);
        if (k_seconds > 0 && elapsed >= k_seconds) {
            stop_reason = "time limit";
            break;
        }
        const bool check_memory = episode % 1000 == 0;
        if (check_memory && resident_mb() > static_cast<double>(k_max_rss_mb)) {
            stop_reason = "memory limit";
            break;
        }
    }

    const double elapsed = seconds_since(start);
    std::printf("\nStopped (%s) after %.1fs: %zu episodes, %zu commitments, rss %.0f MB\n", stop_reason, elapsed,
                episode, commitments, resident_mb());
    std::printf("Whole run: ");
    print_tally(whole_run);
    std::printf("\n");
    if (window.episodes > 0) {
        std::printf("Since the last progress line: ");
        print_tally(window);
        std::printf("\n");
    }
    if (!manifest.best_model_tracker_.best_model().has_value())
        return 0;
    std::printf("Best model, cost %zu:\n", manifest.best_model_tracker_.best_cost());
    size_t lines = 0;
    print_model(manifest.best_model_tracker_.best_model().value(), true_tag, 0, k_model_lines, lines);
    if (lines >= k_model_lines)
        std::printf("  (cut at %zu lines; raise k_model_lines to show more)\n", k_model_lines);
    return 0;
}
