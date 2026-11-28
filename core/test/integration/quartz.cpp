#include <any>
#include <cstddef>
#include <functional>
#include <optional>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/quartz_runtime.hpp"
#include "support/double_vector_hash.hpp"
#include "support/double_vector_length.hpp"

using namespace AML;

namespace {

using rows_t = std::vector<std::vector<bool>>;

struct QuartzIntegrationTest : public ::testing::Test {
    QuartzIntegrationTest();
    native_bin bool_training(const rows_t& inputs, const std::vector<bool>& labels);
    rows_t parity_inputs(size_t input_count);
    std::vector<bool> parity_of(const rows_t& inputs);
    void expect_bool_fit(const quartz_runtime& runtime, const rows_t& inputs, const std::vector<bool>& labels);
    void run(quartz_runtime& runtime, size_t steps);
    native_type boolean;
    native_primitive exclusive_or;
    quartz_options options;
};

QuartzIntegrationTest::QuartzIntegrationTest()
    : boolean(native_type::of<bool, std::hash<bool>>())
    , exclusive_or(native_primitive::of(std::function<bool(bool, bool)>(std::bit_xor<bool>())))
    , options{
          .adds_per_feature = 4, .queue_capacity = 100000, .column_cache_bytes = 1 << 20,
          .constant_cache_bytes = 1 << 10, .seed = 1} {}

// inputs[row][input]
native_bin QuartzIntegrationTest::bool_training(const rows_t& inputs, const std::vector<bool>& labels) {
    const size_t input_count = inputs.empty() ? 0 : inputs[0].size();
    native_bin training{.inputs = {}, .labels = native_column::of(labels), .row_count = labels.size()};
    for (size_t input = 0; input < input_count; ++input) {
        std::vector<bool> cells;
        for (const std::vector<bool>& row : inputs)
            cells.push_back(row[input]);
        training.inputs.push_back(native_column::of(cells));
    }
    return training;
}

rows_t QuartzIntegrationTest::parity_inputs(size_t input_count) {
    rows_t inputs;
    for (size_t assignment = 0; assignment < (size_t{1} << input_count); ++assignment) {
        std::vector<bool> row;
        for (size_t input = 0; input < input_count; ++input)
            row.push_back(((assignment >> input) & 1) == 1);
        inputs.push_back(row);
    }
    return inputs;
}

std::vector<bool> QuartzIntegrationTest::parity_of(const rows_t& inputs) {
    std::vector<bool> labels;
    for (const std::vector<bool>& row : inputs) {
        bool parity = false;
        for (const bool bit : row)
            parity = parity != bit;
        labels.push_back(parity);
    }
    return labels;
}

void QuartzIntegrationTest::expect_bool_fit(
    const quartz_runtime& runtime, const rows_t& inputs, const std::vector<bool>& labels) {
    for (size_t row = 0; row < labels.size(); ++row) {
        std::vector<std::any> values;
        for (const bool cell : inputs[row])
            values.push_back(std::any(cell));
        const std::optional<std::any> label = runtime.infer(values);
        ASSERT_TRUE(label.has_value());
        EXPECT_EQ(std::any_cast<bool>(label.value()), labels[row]);
    }
}

void QuartzIntegrationTest::run(quartz_runtime& runtime, size_t steps) {
    for (size_t step = 0; step < steps; ++step) {
        if (!runtime.next().has_value())
            return;
    }
}

}

TEST_F(QuartzIntegrationTest, NoModelExistsBeforeTheFirstStep) {
    const rows_t inputs = parity_inputs(2);
    quartz_runtime runtime({boolean}, {}, {}, bool_training(inputs, parity_of(inputs)), options);
    EXPECT_FALSE(runtime.best_model().has_value());
}

TEST_F(QuartzIntegrationTest, TheFirstStepVisitsTheVariedInputs) {
    const rows_t inputs = parity_inputs(2);
    quartz_runtime runtime({boolean}, {}, {}, bool_training(inputs, parity_of(inputs)), options);
    const std::optional<table_outcome> outcome = runtime.next();
    ASSERT_TRUE(outcome.has_value());
    // two inputs + four distinct rows
    EXPECT_EQ(outcome.value().cost, 6u);
    EXPECT_EQ(outcome.value().model.graph.outputs.size(), 2u);
    EXPECT_EQ(runtime.best_cost(), 6u);
}

TEST_F(QuartzIntegrationTest, XorWithoutPrimitivesKeepsBothInputsAndThenRunsOutOfSets) {
    const rows_t inputs = parity_inputs(2);
    const std::vector<bool> labels = parity_of(inputs);
    quartz_runtime runtime({boolean}, {}, {}, bool_training(inputs, labels), options);
    runtime.next();
    EXPECT_FALSE(runtime.next().has_value());
    EXPECT_EQ(runtime.best_cost(), 6u);
    expect_bool_fit(runtime, inputs, labels);
}

TEST_F(QuartzIntegrationTest, AgreeingLabelsDropEveryInput) {
    const rows_t inputs = parity_inputs(2);
    const std::vector<bool> labels{true, true, true, true};
    quartz_runtime runtime({boolean}, {}, {}, bool_training(inputs, labels), options);
    run(runtime, 10);
    // no features, one distinct row
    EXPECT_EQ(runtime.best_cost(), 1u);
    EXPECT_TRUE(runtime.best_model().value().graph.outputs.empty());
    expect_bool_fit(runtime, inputs, labels);
}

TEST_F(QuartzIntegrationTest, AnIrrelevantInputIsDropped) {
    const rows_t inputs = parity_inputs(2);
    std::vector<bool> labels;
    for (const std::vector<bool>& row : inputs)
        labels.push_back(row[0]);
    quartz_runtime runtime({boolean}, {}, {}, bool_training(inputs, labels), options);
    run(runtime, 10);
    // x0 + two distinct rows
    EXPECT_EQ(runtime.best_cost(), 3u);
    expect_bool_fit(runtime, inputs, labels);
}

TEST_F(QuartzIntegrationTest, AUniformInputIsNeverInTheStartingSet) {
    const rows_t inputs = parity_inputs(2);
    native_bin training = bool_training(inputs, parity_of(inputs));
    training.inputs.push_back(native_column::of(std::vector<bool>{true, true, true, true}));
    quartz_runtime runtime({boolean}, {}, {}, training, options);
    EXPECT_EQ(runtime.next().value().cost, 6u);
}

TEST_F(QuartzIntegrationTest, XorIsFitByOneDerivedFeature) {
    const rows_t inputs = parity_inputs(2);
    const std::vector<bool> labels = parity_of(inputs);
    quartz_runtime runtime({boolean}, {exclusive_or}, {}, bool_training(inputs, labels), options);
    run(runtime, 50);
    // x0, x1 and their xor + two distinct rows
    EXPECT_EQ(runtime.best_cost(), 5u);
    EXPECT_EQ(runtime.best_model().value().graph.outputs.size(), 1u);
    expect_bool_fit(runtime, inputs, labels);
}

TEST_F(QuartzIntegrationTest, ThreeBitParityIsFitByAChainOfXors) {
    const rows_t inputs = parity_inputs(3);
    const std::vector<bool> labels = parity_of(inputs);
    quartz_runtime runtime({boolean}, {exclusive_or}, {}, bool_training(inputs, labels), options);
    run(runtime, 500);
    // three inputs and two xors + two distinct rows
    EXPECT_EQ(runtime.best_cost(), 7u);
    expect_bool_fit(runtime, inputs, labels);
}

TEST_F(QuartzIntegrationTest, AByteBudgetTooSmallForAnyColumnFindsTheSameModel) {
    const rows_t inputs = parity_inputs(3);
    const std::vector<bool> labels = parity_of(inputs);
    quartz_options starved = options;
    starved.column_cache_bytes = 1;
    starved.constant_cache_bytes = 1;
    quartz_runtime roomy({boolean}, {exclusive_or}, {}, bool_training(inputs, labels), options);
    quartz_runtime cramped({boolean}, {exclusive_or}, {}, bool_training(inputs, labels), starved);
    run(roomy, 500);
    run(cramped, 500);
    EXPECT_EQ(cramped.best_cost(), roomy.best_cost());
    EXPECT_EQ(cramped.best_model().value().graph, roomy.best_model().value().graph);
    expect_bool_fit(cramped, inputs, labels);
}

TEST_F(QuartzIntegrationTest, AnUnseenRowHasNoPrediction) {
    rows_t inputs = parity_inputs(2);
    inputs.pop_back();
    const std::vector<bool> labels = parity_of(inputs);
    quartz_runtime runtime({boolean}, {}, {}, bool_training(inputs, labels), options);
    run(runtime, 10);
    expect_bool_fit(runtime, inputs, labels);
    EXPECT_FALSE(runtime.infer({std::any(true), std::any(true)}).has_value());
}

TEST_F(QuartzIntegrationTest, ConstantsCanBeOperands) {
    const rows_t inputs = parity_inputs(2);
    std::vector<bool> labels;
    for (const std::vector<bool>& row : inputs)
        labels.push_back(!row[0]);
    quartz_runtime runtime(
        {boolean}, {exclusive_or}, {native_constant::of(true)}, bool_training(inputs, labels), options);
    run(runtime, 50);
    // x0 + two distinct rows: the label is a function of x0 alone, and xor with true costs more
    EXPECT_EQ(runtime.best_cost(), 3u);
    expect_bool_fit(runtime, inputs, labels);
}

TEST_F(QuartzIntegrationTest, AConstantBudgetTooSmallForAnyValueFindsTheSameModel) {
    const rows_t inputs = parity_inputs(3);
    std::vector<bool> labels;
    for (const std::vector<bool>& row : inputs)
        labels.push_back(row[0] != row[1]);
    quartz_options starved = options;
    starved.constant_cache_bytes = 0;
    const std::vector<native_constant> constants{native_constant::of(true), native_constant::of(false)};
    quartz_runtime roomy({boolean}, {exclusive_or}, constants, bool_training(inputs, labels), options);
    quartz_runtime cramped({boolean}, {exclusive_or}, constants, bool_training(inputs, labels), starved);
    run(roomy, 200);
    run(cramped, 200);
    EXPECT_EQ(cramped.best_cost(), roomy.best_cost());
    EXPECT_EQ(cramped.best_model().value().graph, roomy.best_model().value().graph);
    expect_bool_fit(cramped, inputs, labels);
}

TEST_F(QuartzIntegrationTest, AQueueOfOneStillVisitsSets) {
    quartz_options narrow = options;
    narrow.queue_capacity = 1;
    const rows_t inputs = parity_inputs(2);
    const std::vector<bool> labels = parity_of(inputs);
    quartz_runtime runtime({boolean}, {exclusive_or}, {}, bool_training(inputs, labels), narrow);
    run(runtime, 50);
    expect_bool_fit(runtime, inputs, labels);
}

TEST_F(QuartzIntegrationTest, ASumOfIntegersIsFitByTheSumAndGeneralizesToUnseenPairs) {
    std::mt19937 rows_engine(3);
    std::uniform_int_distribution<int> digits(0, 9);
    std::vector<int> left;
    std::vector<int> right;
    std::vector<int> sums;
    for (int row = 0; row < 60; ++row) {
        left.push_back(digits(rows_engine));
        right.push_back(digits(rows_engine));
        sums.push_back(left.back() + right.back());
    }
    const native_bin training{
        .inputs = {native_column::of(left), native_column::of(right)}, .labels = native_column::of(sums),
        .row_count = sums.size()};
    const native_primitive plus = native_primitive::of(std::function<int(int, int)>(std::plus<int>()));
    const native_primitive times = native_primitive::of(std::function<int(int, int)>(std::multiplies<int>()));
    quartz_runtime runtime({native_type::of<int, std::hash<int>>()}, {plus, times}, {}, training, options);
    run(runtime, 300);
    EXPECT_EQ(runtime.best_model().value().graph.outputs.size(), 1u);
    for (size_t row = 0; row < sums.size(); ++row)
        EXPECT_EQ(std::any_cast<int>(runtime.infer({std::any(left[row]), std::any(right[row])}).value()), sums[row]);
    // a pair whose sum some row held, swapped
    EXPECT_EQ(std::any_cast<int>(runtime.infer({std::any(right[0]), std::any(left[0])}).value()), sums[0]);
}

TEST_F(QuartzIntegrationTest, AVectorTypeWithItsOwnHashIsLearnedThrough) {
    std::mt19937 rows_engine(5);
    std::uniform_int_distribution<int> lengths(0, 4);
    std::uniform_real_distribution<double> elements(-1.0, 1.0);
    std::vector<std::vector<double>> vectors;
    std::vector<int> labels;
    for (int row = 0; row < 40; ++row) {
        std::vector<double> drawn;
        const int length = lengths(rows_engine);
        for (int element = 0; element < length; ++element)
            drawn.push_back(elements(rows_engine));
        vectors.push_back(drawn);
        labels.push_back(length);
    }
    const native_bin training{
        .inputs = {native_column::of(vectors)}, .labels = native_column::of(labels), .row_count = labels.size()};
    const native_primitive length = native_primitive::of(
        std::function<int(const std::vector<double>&)>(double_vector_length()));
    quartz_runtime runtime(
        {native_type::of<std::vector<double>, double_vector_hash>(), native_type::of<int, std::hash<int>>()},
        {length}, {}, training, options);
    run(runtime, 50);
    // the vector and its length + five distinct lengths
    EXPECT_EQ(runtime.best_cost(), 7u);
    EXPECT_EQ(std::any_cast<int>(runtime.infer({std::any(std::vector<double>{9.0, 9.0})}).value()), 2);
}
