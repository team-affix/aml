#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/statement_processor.hpp"
#include "value_objects/data_point.hpp"
#include "value_objects/lc_expr.hpp"
#include "value_objects/statement.hpp"

namespace {

struct MockTranspileExpr {
    MOCK_METHOD(const lc_expr*, transpile, (const aml_expr*), ());
};
struct MockPushDataPoint {
    MOCK_METHOD(void, push_data_point, (data_point), ());
};

using NiceTx   = testing::NiceMock<MockTranspileExpr>;
using NicePush = testing::NiceMock<MockPushDataPoint>;

struct StatementProcessorTest : public ::testing::Test {
    lc_expr  in_term{};
    lc_expr  out_term{};
    NiceTx   tx;
    NicePush push;
    statement_processor<NiceTx, NicePush> sp{tx, push};
};

} // namespace

TEST_F(StatementProcessorTest, TranspilesAndPushesStatement) {
    using testing::Return;

    aml_expr lhs{};
    aml_expr rhs{};
    EXPECT_CALL(tx, transpile(&lhs)).WillOnce(Return(&in_term));
    EXPECT_CALL(tx, transpile(&rhs)).WillOnce(Return(&out_term));
    EXPECT_CALL(push, push_data_point(data_point{&in_term, &out_term})).Times(1);

    sp.process_statement(statement{&lhs, &rhs});
}
