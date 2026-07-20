#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include "infrastructure/statement_pump.hpp"
#include "value_objects/statement.hpp"

namespace {

struct MockGetNextStatement {
    MOCK_METHOD(std::optional<statement>, get_next_statement, (), ());
};
struct MockProcessStatement {
    MOCK_METHOD(void, process_statement, (const statement&), ());
};

using NiceNext = testing::NiceMock<MockGetNextStatement>;
using NiceProc = testing::NiceMock<MockProcessStatement>;

struct StatementPumpTest : public ::testing::Test {
    NiceNext next;
    NiceProc proc;
    statement_pump<NiceNext, NiceProc> pump_{next, proc};
};

} // namespace

TEST_F(StatementPumpTest, EmptyIteratorDoesNothing) {
    using testing::Return;

    EXPECT_CALL(next, get_next_statement()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(proc, process_statement(testing::_)).Times(0);
    pump_.pump();
}

TEST_F(StatementPumpTest, SingleStatementIsProcessed) {
    using testing::Return;
    using testing::InSequence;

    aml_expr lhs{};
    aml_expr rhs{};
    statement s{&lhs, &rhs};
    {
        InSequence seq;
        EXPECT_CALL(next, get_next_statement()).WillOnce(Return(s));
        EXPECT_CALL(proc, process_statement(s));
        EXPECT_CALL(next, get_next_statement()).WillOnce(Return(std::nullopt));
    }
    pump_.pump();
}

TEST_F(StatementPumpTest, PumpsEachStatementUntilExhausted) {
    using testing::Return;
    using testing::InSequence;

    aml_expr lhs0{};
    aml_expr rhs0{};
    aml_expr lhs1{};
    aml_expr rhs1{};
    statement s0{&lhs0, &rhs0};
    statement s1{&lhs1, &rhs1};

    {
        InSequence seq;
        EXPECT_CALL(next, get_next_statement()).WillOnce(Return(s0));
        EXPECT_CALL(proc, process_statement(s0));
        EXPECT_CALL(next, get_next_statement()).WillOnce(Return(s1));
        EXPECT_CALL(proc, process_statement(s1));
        EXPECT_CALL(next, get_next_statement()).WillOnce(Return(std::nullopt));
    }
    pump_.pump();
}
