// The xor ladder: how much does each nested xor at the root of a 10-bit parity model improve the reward an MCTS
// rollout sees? The root is fixed to match on xor(...xor(xor(x0, x1), x2)..., xk); everything below it is built
// exactly as a Razor rollout builds it, with every choice uniformly random. Prints the cost distribution per k.
//
// Build and run from the repository root, as README.md shows:
//   build/xor_ladder <max_applications> <seed> <samples_per_root>      e.g. build/xor_ladder 9 1 2000

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "infrastructure/razor_manifest.hpp"
#include "infrastructure/schema.hpp"
#include "infrastructure/value_pool.hpp"
#include "support/bool_and.hpp"
#include "support/bool_not.hpp"
#include "support/bool_or.hpp"

namespace {

struct bool_xor {
    value_pool& values;
    size_t false_tag;
    size_t true_tag;
    const value* operator()(const std::vector<const value*>& args) const {
        const bool result = (args[0]->tag == true_tag) != (args[1]->tag == true_tag);
        return values.make(result ? true_tag : false_tag, {});
    }
};

}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::fprintf(stderr, "usage: %s <max_applications> <seed> <samples_per_root>\n", argv[0]);
        return 1;
    }
    const size_t bits = 10;
    const size_t max_applications = std::atoi(argv[1]);
    const uint32_t seed = std::atoi(argv[2]);
    const size_t samples = std::atoi(argv[3]);

    const type_id boolean = 0;
    const size_t false_tag = 0;
    const size_t true_tag = 1;
    schema types(std::vector<type_def>{type_def{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {}}}}});
    value_pool values;
    const value* falsity = values.make(false_tag, {});
    const value* truth = values.make(true_tag, {});
    const primitive_type binary{.arg_types = {boolean, boolean}, .result_type = boolean};
    // Primitive 0 is xor; and, or and not are offered too, as in the parity runs.
    const std::vector<typed_primitive> primitives{
        {.type = binary, .apply = bool_xor{values, false_tag, true_tag}},
        {.type = binary, .apply = bool_and<value_pool>(values, false_tag, true_tag)},
        {.type = binary, .apply = bool_or<value_pool>(values, false_tag, true_tag)},
        {.type = {.arg_types = {boolean}, .result_type = boolean},
         .apply = bool_not<value_pool>(values, false_tag, true_tag)}};
    const std::vector<typed_constant> constants{
        {.type = boolean, .literal = falsity}, {.type = boolean, .literal = truth}};

    // The full truth table: 1024 rows, x0..x9 are the bits and the label is their parity.
    bin training{.inputs = {}, .labels = typed_column{.type = boolean, .cells = {}}};
    for (size_t bit = 0; bit < bits; ++bit)
        training.inputs.push_back(typed_column{.type = boolean, .cells = {}});
    for (size_t row = 0; row < (1u << bits); ++row) {
        bool parity = false;
        for (size_t bit = 0; bit < bits; ++bit) {
            const bool set = (row >> bit) & 1;
            parity ^= set;
            training.inputs[bit].cells.push_back(set ? truth : falsity);
        }
        training.labels.cells.push_back(parity ? truth : falsity);
    }

    // The exploration constant plays no part: every choice is made inside a rollout.
    razor_manifest solver(
        types, values, primitives,
        razor_options{.max_applications = max_applications, .exploration_constant = 75, .seed = seed});
    solver.constant_seeder_.seed(constants);

    feature root_feature{feature::var{.index = 0}};
    for (size_t xors = 0; xors < bits; ++xors) {
        if (xors > 0) {
            const feature next_input{feature::var{.index = static_cast<var_id>(xors)}};
            root_feature = feature{feature::app{.prim = 0, .args = {root_feature, next_input}}};
        }
        typed_column observed{.type = boolean, .cells = {}};
        for (size_t row = 0; row < training.labels.cells.size(); ++row) {
            std::vector<const value*> scope;
            for (const typed_column& input : training.inputs)
                scope.push_back(input.cells[row]);
            observed.cells.push_back(solver.feature_evaluator_.evaluate(root_feature, scope));
        }
        const std::vector<bin> children = solver.bin_splitter_.split(training, observed);
        const size_t root_cost = 1 + solver.feature_sizer_.size(root_feature);

        std::vector<double> costs;
        for (size_t sample = 0; sample < samples; ++sample) {
            solver.in_rollout_.enter_rollout();
            size_t cost = root_cost;
            for (const bin& child : children)
                cost += solver.model_sizer_.size(solver.model_builder_.build(child));
            solver.in_rollout_.exit_rollout();
            costs.push_back(static_cast<double>(cost));
        }
        double sum = 0;
        double squares = 0;
        for (const double cost : costs) {
            sum += cost;
            squares += cost * cost;
        }
        const double mean = sum / costs.size();
        const double deviation = std::sqrt(squares / costs.size() - mean * mean);
        std::sort(costs.begin(), costs.end());
        std::printf(
            "xors in root %zu (size %2zu): mean %7.1f  sd %5.1f  se %4.1f  min %5.0f  p10 %6.0f  median %6.0f\n",
            xors, solver.feature_sizer_.size(root_feature), mean, deviation, deviation / std::sqrt(costs.size()),
            costs.front(), costs[costs.size() / 10], costs[costs.size() / 2]);
        std::fflush(stdout);
    }
}
