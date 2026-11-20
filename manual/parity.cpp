// Manual parity run: trains Falcon on n-bit parity (every row of the truth table) and prints its progress until a
// time limit, a memory limit or Ctrl-C, then a summary with the best model found. Not part of the core library; see
// manual/README.md.

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <variant>
#include <vector>
#include <unistd.h>
#include "infrastructure/falcon_manifest.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/value_pool.hpp"
#include "manual/bool_xor.hpp"
#include "support/bool_and.hpp"
#include "support/bool_not.hpp"
#include "support/bool_or.hpp"

namespace {

const size_t k_false_tag = 0;
const size_t k_true_tag = 1;
const type_id k_boolean = 0;
const prim_id k_xor = 0;
const char* const k_primitive_names[] = {"xor", "and", "or", "not"};
const size_t k_xor_buckets = 6;

volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) {
    stop_requested = 1;
}

struct run_settings {
    size_t bits;
    size_t max_applications;
    double exploration_constant;
    uint32_t seed;
    size_t commit_visits;
    double seconds;
    double report_seconds;
    size_t max_rss_mb;
    size_t model_lines;
};

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

void print_usage() {
    std::printf(
        "usage: manual_parity [--key=value ...]\n"
        "  --bits=10              parity over this many input bits (the training set is all 2^bits rows)\n"
        "  --max-applications=9   synthesis applications per session\n"
        "  --c=4                  exploration constant, on ucb1's normalized [0, 1] scale (may exceed 1)\n"
        "  --seed=1               random seed\n"
        "  --commit-visits=52000  visits at the frontier before committing along the best path (0: never commit)\n"
        "  --seconds=0            stop after this many seconds (0: run until Ctrl-C)\n"
        "  --report-seconds=10    seconds between progress lines\n"
        "  --max-rss-mb=4000      stop if resident memory exceeds this many MB\n"
        "  --model-lines=40       lines of the best model to print at the end\n");
}

bool parse_option(const char* argument, run_settings& settings) {
    const char* equals = std::strchr(argument, '=');
    if (std::strncmp(argument, "--", 2) != 0 || equals == nullptr)
        return false;
    const std::string key(argument + 2, equals);
    const char* text = equals + 1;
    if (key == "bits") settings.bits = std::strtoull(text, nullptr, 10);
    else if (key == "max-applications") settings.max_applications = std::strtoull(text, nullptr, 10);
    else if (key == "c") settings.exploration_constant = std::strtod(text, nullptr);
    else if (key == "seed") settings.seed = static_cast<uint32_t>(std::strtoul(text, nullptr, 10));
    else if (key == "commit-visits") settings.commit_visits = std::strtoull(text, nullptr, 10);
    else if (key == "seconds") settings.seconds = std::strtod(text, nullptr);
    else if (key == "report-seconds") settings.report_seconds = std::strtod(text, nullptr);
    else if (key == "max-rss-mb") settings.max_rss_mb = std::strtoull(text, nullptr, 10);
    else if (key == "model-lines") settings.model_lines = std::strtoull(text, nullptr, 10);
    else return false;
    return true;
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

size_t count_xors(const feature& expression) {
    const feature::app* application = std::get_if<feature::app>(&expression.content);
    if (application == nullptr)
        return 0;
    size_t total = application->prim == k_xor ? 1 : 0;
    for (const feature& arg : application->args)
        total += count_xors(arg);
    return total;
}

std::string show_feature(const feature& expression) {
    if (const feature::var* variable = std::get_if<feature::var>(&expression.content))
        return "x" + std::to_string(variable->index);
    if (const feature::constant* constant = std::get_if<feature::constant>(&expression.content))
        return constant->literal->tag == k_true_tag ? "T" : "F";
    const feature::app& application = std::get<feature::app>(expression.content);
    std::string text = std::string(k_primitive_names[application.prim]) + "(";
    for (size_t index = 0; index < application.args.size(); ++index)
        text += (index == 0 ? "" : ", ") + show_feature(application.args[index]);
    return text + ")";
}

std::string show_root(const model_node& model) {
    if (const model_node::match* match = std::get_if<model_node::match>(&model.content))
        return "match " + show_feature(match->scrutinee);
    if (const model_node::leaf* leaf = std::get_if<model_node::leaf>(&model.content))
        return "leaf " + show_feature(leaf->output);
    return "empty";
}

size_t root_xors(const model_node& model) {
    const model_node::match* match = std::get_if<model_node::match>(&model.content);
    if (match == nullptr)
        return 0;
    return count_xors(match->scrutinee);
}

void print_model(const model_node& node, size_t depth, size_t max_lines, size_t& lines) {
    if (lines >= max_lines)
        return;
    const std::string indent(2 * depth + 2, ' ');
    ++lines;
    const model_node::match* match = std::get_if<model_node::match>(&node.content);
    if (match == nullptr) {
        std::printf("%s%s\n", indent.c_str(), show_root(node).c_str());
        return;
    }
    std::printf("%smatch %s\n", indent.c_str(), show_feature(match->scrutinee).c_str());
    for (size_t branch = 0; branch < match->branches.size(); ++branch) {
        if (lines >= max_lines)
            return;
        ++lines;
        std::printf("%s  %s ->\n", indent.c_str(), branch == k_true_tag ? "T" : "F");
        print_model(match->branches[branch], depth + 2, max_lines, lines);
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

bin parity_training(value_pool& values, size_t bits) {
    const value* falsity = values.make(k_false_tag, {});
    const value* truth = values.make(k_true_tag, {});
    bin training{.inputs = {}, .labels = typed_column{.type = k_boolean, .cells = {}}};
    for (size_t bit = 0; bit < bits; ++bit)
        training.inputs.push_back(typed_column{.type = k_boolean, .cells = {}});
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

int main(int argc, char** argv) {
    run_settings settings{
        .bits = 10,
        .max_applications = 9,
        .exploration_constant = 4.0,
        .seed = 1,
        .commit_visits = 52000,
        .seconds = 0,
        .report_seconds = 10,
        .max_rss_mb = 4000,
        .model_lines = 40};
    for (int index = 1; index < argc; ++index) {
        if (parse_option(argv[index], settings))
            continue;
        print_usage();
        return std::strcmp(argv[index], "--help") == 0 ? 0 : 1;
    }
    const size_t commit_visits =
        settings.commit_visits == 0 ? std::numeric_limits<size_t>::max() : settings.commit_visits;

    schema types(std::vector<type_def>{type_def{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {}}}}});
    value_pool values;
    const primitive_type binary{.arg_types = {k_boolean, k_boolean}, .result_type = k_boolean};
    const std::vector<typed_primitive> primitives{
        {.type = binary, .apply = bool_xor<value_pool>(values, k_false_tag, k_true_tag)},
        {.type = binary, .apply = bool_and<value_pool>(values, k_false_tag, k_true_tag)},
        {.type = binary, .apply = bool_or<value_pool>(values, k_false_tag, k_true_tag)},
        {.type = {.arg_types = {k_boolean}, .result_type = k_boolean},
         .apply = bool_not<value_pool>(values, k_false_tag, k_true_tag)}};
    const std::vector<typed_constant> constants{
        {.type = k_boolean, .literal = values.make(k_false_tag, {})},
        {.type = k_boolean, .literal = values.make(k_true_tag, {})}};
    const bin training = parity_training(values, settings.bits);

    falcon_manifest solver(types, values, primitives,
                           falcon_options{.max_applications = settings.max_applications,
                                          .exploration_constant = settings.exploration_constant,
                                          .seed = settings.seed,
                                          .commit_visits = commit_visits});
    solver.constant_seeder_.seed(constants);

    std::signal(SIGINT, request_stop);
    std::printf("Falcon on %zu-bit parity (%zu rows): max applications %zu, c %.3g, seed %u, commit visits %s\n",
                settings.bits, training.labels.cells.size(), settings.max_applications,
                settings.exploration_constant, settings.seed,
                settings.commit_visits == 0 ? "never" : std::to_string(settings.commit_visits).c_str());
    std::printf("Primitives: xor, and, or, not. Constants: F, T. Ctrl-C stops and prints a summary.\n\n");
    std::fflush(stdout);

    const auto start = std::chrono::steady_clock::now();
    root_tally whole_run = empty_tally();
    root_tally window = empty_tally();
    size_t episode = 0;
    size_t best_cost = std::numeric_limits<size_t>::max();
    size_t commitments = 0;
    double next_report = settings.report_seconds;
    const char* stop_reason = "Ctrl-C";
    while (stop_requested == 0) {
        // One episode, as falcon's mcts_episode runs it, keeping the model to look at its root.
        const model_node model = solver.model_builder_.build(training);
        const size_t cost = solver.model_sizer_.size(model);
        solver.best_model_tracker_.try_record(model, cost);
        solver.value_delta_.set_value(-static_cast<double>(cost));
        solver.terminator_.terminate();
        ++episode;
        const size_t xors = root_xors(model);
        record_episode(whole_run, xors, cost);
        record_episode(window, xors, cost);

        const double elapsed = seconds_since(start);
        if (solver.best_model_tracker_.best_cost() < best_cost) {
            best_cost = solver.best_model_tracker_.best_cost();
            std::printf("  new best %zu at %.1fs (episode %zu): %s\n", best_cost, elapsed, episode,
                        show_root(solver.best_model_tracker_.best_model().value()).c_str());
        }
        if (solver.commitments_.commitment_count() != commitments) {
            commitments = solver.commitments_.commitment_count();
            std::printf("  commitment %zu at %.1fs (episode %zu)\n", commitments, elapsed, episode);
        }
        if (elapsed >= next_report) {
            next_report += settings.report_seconds;
            std::printf("[%6.0fs] episodes %zu (%.0f/s) | rss %.0f MB | best %zu | commitments %zu | last %.0fs: ",
                        elapsed, episode, static_cast<double>(episode) / elapsed, resident_mb(), best_cost,
                        commitments, settings.report_seconds);
            print_tally(window);
            std::printf("\n");
            window = empty_tally();
        }
        std::fflush(stdout);
        if (settings.seconds > 0 && elapsed >= settings.seconds) {
            stop_reason = "time limit";
            break;
        }
        const bool check_memory = episode % 1000 == 0;
        if (check_memory && resident_mb() > static_cast<double>(settings.max_rss_mb)) {
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
    if (!solver.best_model_tracker_.best_model().has_value())
        return 0;
    std::printf("Best model, cost %zu:\n", solver.best_model_tracker_.best_cost());
    size_t lines = 0;
    print_model(solver.best_model_tracker_.best_model().value(), 0, settings.model_lines, lines);
    if (lines >= settings.model_lines)
        std::printf("  (cut at %zu lines; --model-lines shows more)\n", settings.model_lines);
    return 0;
}
