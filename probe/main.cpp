// Probe: a developer's entry point for watching a solver run, outside the core library. Swap this file's example for
// any other. The current example trains Falcon to tell odd-length boolean lists (true) from even-length ones (false),
// from every list up to a length, with list primitives (length among them) and natural-number primitives (modulo). It
// prints progress until a time limit, a memory limit or Ctrl-C, then the best model, whether it fits the training
// lists, and how it does on longer, unseen lists.
// See probe/README.md.

#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <unistd.h>
#include "infrastructure/falcon_manifest.hpp"
#include "infrastructure/schema.hpp"
#include "probe/bool_xor.hpp"
#include "probe/list_primitives.hpp"
#include "probe/nat_mod.hpp"
#include "support/bool_not.hpp"

namespace {

const char* const k_primitive_names[] = {"is_nil", "head", "tail", "length", "mod", "not", "xor"};

// Settings for this example: edit them here.
const size_t k_max_training_length = 6;  // training lists: every boolean list of length 0 up to this
const size_t k_max_unseen_length = 9;    // unseen lists for generalization: every length above training, up to this
const size_t k_max_applications = 4;     // synthesis applications per session
const double k_exploration_constant = 4; // on ucb1's normalized [0, 1] scale; above 1 favours exploration
const uint32_t k_seed = 1;
const size_t k_commit_visits = 0;        // frontier visits before committing along the best path; 0 never commits
const double k_seconds = 0;              // stop after this many seconds; 0 runs until Ctrl-C
const double k_report_seconds = 10;      // seconds between progress lines
const size_t k_max_rss_mb = 4000;        // stop once resident memory passes this (checked every 1,000 episodes)
const size_t k_model_lines = 60;         // lines of the best model printed at the end

volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) {
    stop_requested = 1;
}

// The user's types and the values the example names when printing.
struct list_types {
    AML::type_id boolean;
    AML::ctor_id false_ctor;
    AML::ctor_id true_ctor;
    AML::type_id list;
    AML::ctor_id nil_ctor;
    AML::ctor_id cons_ctor;
    AML::type_id nat;
    AML::ctor_id zero_ctor;
    AML::ctor_id suc_ctor;
    AML::value falsity;
    AML::value truth;
    AML::value nil;
};

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

// Values carry no type, so F, nil and 0 are the same value; it prints as 0. A chain of successors prints as its number.
std::string show_value(const AML::value& term, const list_types& types) {
    if (term == types.truth)
        return "T";
    size_t successors = 0;
    const AML::value* rest = &term;
    while (rest->ctor == types.suc_ctor && rest->args.size() == 1) {
        ++successors;
        rest = &rest->args[0];
    }
    if (rest->ctor == types.zero_ctor && rest->args.empty())
        return std::to_string(successors);
    std::string text = "c" + std::to_string(term.ctor) + "(";
    for (size_t index = 0; index < term.args.size(); ++index)
        text += (index == 0 ? "" : ", ") + show_value(term.args[index], types);
    return text + ")";
}

std::string show_feature(const AML::feature& expression, const list_types& types) {
    if (const AML::feature::var* variable = std::get_if<AML::feature::var>(&expression.content))
        return "x" + std::to_string(variable->index);
    if (const AML::feature::constant* constant = std::get_if<AML::feature::constant>(&expression.content))
        return show_value(constant->literal, types);
    const AML::feature::app& application = std::get<AML::feature::app>(expression.content);
    std::string text = std::string(k_primitive_names[application.prim]) + "(";
    for (size_t index = 0; index < application.args.size(); ++index)
        text += (index == 0 ? "" : ", ") + show_feature(application.args[index], types);
    return text + ")";
}

std::string show_root(const AML::model_node& model, const list_types& types) {
    if (const AML::model_node::match* match = std::get_if<AML::model_node::match>(&model.content))
        return "match " + show_feature(match->scrutinee, types);
    if (const AML::model_node::leaf* leaf = std::get_if<AML::model_node::leaf>(&model.content))
        return "leaf " + show_feature(leaf->output, types);
    return "empty";
}

// The branch labels of a match on a list or a boolean: the scrutinee's type is not stored, so a two-way match is
// labelled by the constructor numbers both types share.
void print_model(const AML::model_node& node, const list_types& types, size_t depth, size_t max_lines,
                 size_t& lines) {
    if (lines >= max_lines)
        return;
    const std::string indent(2 * depth + 2, ' ');
    ++lines;
    const AML::model_node::match* match = std::get_if<AML::model_node::match>(&node.content);
    if (match == nullptr) {
        std::printf("%s%s\n", indent.c_str(), show_root(node, types).c_str());
        return;
    }
    std::printf("%smatch %s\n", indent.c_str(), show_feature(match->scrutinee, types).c_str());
    for (size_t branch = 0; branch < match->branches.size(); ++branch) {
        if (lines >= max_lines)
            return;
        ++lines;
        std::printf("%s  case %zu ->\n", indent.c_str(), branch);
        print_model(match->branches[branch], types, depth + 2, max_lines, lines);
    }
}

// Every boolean list of exactly `length` elements, in a fixed order.
std::vector<AML::value> lists_of_length(size_t length, const list_types& types) {
    std::vector<AML::value> lists{types.nil};
    for (size_t step = 0; step < length; ++step) {
        std::vector<AML::value> longer;
        for (const AML::value& rest : lists) {
            longer.push_back(AML::value{.ctor = types.cons_ctor, .args = {types.falsity, rest}});
            longer.push_back(AML::value{.ctor = types.cons_ctor, .args = {types.truth, rest}});
        }
        lists = longer;
    }
    return lists;
}

AML::bin odd_length_training(const list_types& types) {
    AML::bin training{.inputs = {AML::typed_column{.type = types.list, .cells = {}}},
                      .labels = AML::typed_column{.type = types.boolean, .cells = {}}};
    for (size_t length = 0; length <= k_max_training_length; ++length) {
        for (const AML::value& list : lists_of_length(length, types)) {
            training.inputs[0].cells.push_back(list);
            training.labels.cells.push_back(length % 2 == 1 ? types.truth : types.falsity);
        }
    }
    return training;
}

// How a model does on every list of one length: right, wrong, or no answer (an empty branch).
struct length_score {
    size_t right;
    size_t wrong;
    size_t unanswered;
};

template<typename IEvaluateModel>
length_score score_length(IEvaluateModel& evaluate_model, const AML::model_node& model, size_t length,
                          const list_types& types) {
    length_score score{.right = 0, .wrong = 0, .unanswered = 0};
    const AML::value expected = length % 2 == 1 ? types.truth : types.falsity;
    for (const AML::value& list : lists_of_length(length, types)) {
        const std::optional<AML::value> answer = evaluate_model.evaluate(model, {list});
        if (!answer.has_value())
            ++score.unanswered;
        else if (answer.value() == expected)
            ++score.right;
        else
            ++score.wrong;
    }
    return score;
}

double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

}

int main() {
    const size_t commit_visits = k_commit_visits == 0 ? std::numeric_limits<size_t>::max() : k_commit_visits;

    AML::schema schema;
    list_types types{};
    types.boolean = schema.declare_type();
    types.false_ctor = schema.declare_ctor(types.boolean, {});
    types.true_ctor = schema.declare_ctor(types.boolean, {});
    types.list = schema.declare_type();
    types.nil_ctor = schema.declare_ctor(types.list, {});
    types.cons_ctor = schema.declare_ctor(types.list, {types.boolean, types.list});
    types.nat = schema.declare_type();
    types.zero_ctor = schema.declare_ctor(types.nat, {});
    types.suc_ctor = schema.declare_ctor(types.nat, {types.nat});
    types.falsity = AML::value{.ctor = types.false_ctor, .args = {}};
    types.truth = AML::value{.ctor = types.true_ctor, .args = {}};
    types.nil = AML::value{.ctor = types.nil_ctor, .args = {}};

    const std::vector<AML::typed_primitive> primitives{
        {.type = {.arg_types = {types.list}, .result_type = types.boolean},
         .apply = list_is_nil(types.nil_ctor, types.false_ctor, types.true_ctor)},
        {.type = {.arg_types = {types.list}, .result_type = types.boolean},
         .apply = list_head(types.nil_ctor, types.falsity)},
        {.type = {.arg_types = {types.list}, .result_type = types.list}, .apply = list_tail(types.nil_ctor)},
        {.type = {.arg_types = {types.list}, .result_type = types.nat},
         .apply = list_length(types.nil_ctor, types.zero_ctor, types.suc_ctor)},
        {.type = {.arg_types = {types.nat, types.nat}, .result_type = types.nat},
         .apply = nat_mod(types.zero_ctor, types.suc_ctor)},
        {.type = {.arg_types = {types.boolean}, .result_type = types.boolean},
         .apply = bool_not(types.false_ctor, types.true_ctor)},
        {.type = {.arg_types = {types.boolean, types.boolean}, .result_type = types.boolean},
         .apply = bool_xor(types.false_ctor, types.true_ctor)}};
    const AML::value zero{.ctor = types.zero_ctor, .args = {}};
    const AML::value one{.ctor = types.suc_ctor, .args = {zero}};
    const AML::value two{.ctor = types.suc_ctor, .args = {one}};
    const std::vector<AML::typed_constant> constants{
        {.type = types.boolean, .literal = types.falsity},
        {.type = types.boolean, .literal = types.truth},
        {.type = types.list, .literal = types.nil},
        {.type = types.nat, .literal = zero},
        {.type = types.nat, .literal = one},
        {.type = types.nat, .literal = two}};
    const AML::bin training = odd_length_training(types);

    // The manifest directly, not falcon_runtime: the probe also watches the commitments, which no runtime exposes.
    AML::falcon_manifest manifest(schema, primitives,
                                  AML::falcon_options{.max_applications = k_max_applications,
                                                      .exploration_constant = k_exploration_constant,
                                                      .seed = k_seed,
                                                      .commit_visits = commit_visits});
    manifest.constant_seeder_.seed(constants);

    std::signal(SIGINT, request_stop);
    std::printf("Falcon on odd-length boolean lists: %zu training lists (lengths 0 to %zu), max applications %zu, "
                "c %.3g, seed %u, commit visits %s\n",
                training.labels.cells.size(), k_max_training_length, k_max_applications, k_exploration_constant,
                k_seed, k_commit_visits == 0 ? "never" : std::to_string(k_commit_visits).c_str());
    std::printf("Primitives: is_nil, head, tail, length, mod, not, xor. Constants: F, T, nil, 0, 1, 2. "
                "Ctrl-C stops and prints a summary.\n\n");
    std::fflush(stdout);

    const auto start = std::chrono::steady_clock::now();
    size_t episode = 0;
    size_t best_cost = std::numeric_limits<size_t>::max();
    size_t commitments = 0;
    double window_cost = 0;
    size_t window_episodes = 0;
    double next_report = k_report_seconds;
    const char* stop_reason = "Ctrl-C";
    while (stop_requested == 0) {
        const AML::episode_outcome outcome = manifest.mcts_episode_.run(training);
        ++episode;
        window_cost += static_cast<double>(outcome.cost);
        ++window_episodes;

        const double elapsed = seconds_since(start);
        if (manifest.best_model_tracker_.best_cost() < best_cost) {
            best_cost = manifest.best_model_tracker_.best_cost();
            std::printf("  new best %zu at %.1fs (episode %zu): %s\n", best_cost, elapsed, episode,
                        show_root(manifest.best_model_tracker_.best_model().value(), types).c_str());
        }
        if (manifest.commitments_.commitment_count() != commitments) {
            commitments = manifest.commitments_.commitment_count();
            std::printf("  commitment %zu at %.1fs (episode %zu)\n", commitments, elapsed, episode);
        }
        if (elapsed >= next_report) {
            next_report += k_report_seconds;
            std::printf("[%6.0fs] episodes %zu (%.0f/s) | rss %.0f MB | best %zu | commitments %zu | "
                        "mean cost of the last %zu: %.0f\n",
                        elapsed, episode, static_cast<double>(episode) / elapsed, resident_mb(), best_cost,
                        commitments, window_episodes, window_cost / static_cast<double>(window_episodes));
            window_cost = 0;
            window_episodes = 0;
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

    std::printf("\nStopped (%s) after %.1fs: %zu episodes, %zu commitments, rss %.0f MB\n", stop_reason,
                seconds_since(start), episode, commitments, resident_mb());
    if (!manifest.best_model_tracker_.best_model().has_value())
        return 0;
    const AML::model_node& best = manifest.best_model_tracker_.best_model().value();
    std::printf("Best model, cost %zu (list matches: case 0 is nil, case 1 is cons(head, tail); natural matches: "
                "case 0 is zero, case 1 is suc(predecessor); boolean matches: case 0 is F, case 1 is T):\n",
                manifest.best_model_tracker_.best_cost());
    size_t lines = 0;
    print_model(best, types, 0, k_model_lines, lines);
    if (lines >= k_model_lines)
        std::printf("  (cut at %zu lines; raise k_model_lines to show more)\n", k_model_lines);

    std::printf("\nBy list length (right / wrong / no answer):\n");
    for (size_t length = 0; length <= k_max_unseen_length; ++length) {
        const length_score score = score_length(manifest.model_evaluator_, best, length, types);
        std::printf("  length %zu%s: %zu / %zu / %zu\n", length, length > k_max_training_length ? " (unseen)" : "",
                    score.right, score.wrong, score.unanswered);
    }
    return 0;
}
