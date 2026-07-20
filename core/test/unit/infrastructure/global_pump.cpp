#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include "infrastructure/global_pump.hpp"
#include "value_objects/declaration.hpp"
#include "value_objects/declaration_group.hpp"
#include "value_objects/definition.hpp"
#include "value_objects/global.hpp"

namespace {

struct MockGetNextGlobal {
    MOCK_METHOD(std::optional<global>, get_next_global, (), ());
};
struct MockProcessGlobal {
    MOCK_METHOD(void, process_global, (const global&), ());
};

using NiceNext = testing::NiceMock<MockGetNextGlobal>;
using NiceProc = testing::NiceMock<MockProcessGlobal>;

declaration_group make_group(std::initializer_list<declaration> decls) {
    declaration_group g;
    g.declarations = decls;
    return g;
}

struct GlobalPumpTest : public ::testing::Test {
    NiceNext next;
    NiceProc proc;
    global_pump<NiceNext, NiceProc> pump_{next, proc};
};

} // namespace

TEST_F(GlobalPumpTest, EmptyIteratorDoesNothing) {
    using testing::Return;

    EXPECT_CALL(next, get_next_global()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(proc, process_global(testing::_)).Times(0);
    pump_.pump();
}

TEST_F(GlobalPumpTest, SingleGlobalIsProcessed) {
    using testing::Return;
    using testing::InSequence;

    aml_expr body{};
    global item{definition{"f", &body}};
    {
        InSequence seq;
        EXPECT_CALL(next, get_next_global()).WillOnce(Return(item));
        EXPECT_CALL(proc, process_global(item));
        EXPECT_CALL(next, get_next_global()).WillOnce(Return(std::nullopt));
    }
    pump_.pump();
}

TEST_F(GlobalPumpTest, PumpsEachGlobalUntilExhausted) {
    using testing::Return;
    using testing::InSequence;

    aml_expr body{};
    global def_item{definition{"f", &body}};
    global decl_item{make_group({{"true", 0u}})};

    {
        InSequence seq;
        EXPECT_CALL(next, get_next_global()).WillOnce(Return(def_item));
        EXPECT_CALL(proc, process_global(def_item));
        EXPECT_CALL(next, get_next_global()).WillOnce(Return(decl_item));
        EXPECT_CALL(proc, process_global(decl_item));
        EXPECT_CALL(next, get_next_global()).WillOnce(Return(std::nullopt));
    }
    pump_.pump();
}
