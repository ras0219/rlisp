#include "cons.h"
#include "mempool.h"
#include "parser.h"
#include "testutil.h"
#include "vcpkgparser.h"
#include <gtest/gtest.h>

using namespace rlisp;
TEST(Parser, FailOnEmpty)
{
    rlisp::MemPool mempool;
    vcpkg::Parse::ParserBase parser("", "origin");
    rlisp::parse(parser, mempool);
    EXPECT_NE(nullptr, parser.get_error());
    EXPECT_EQ(mempool.num_roots(), 0);
}
TEST(Parser, SucceedOnEmptyList)
{
    rlisp::MemPool mempool;
    vcpkg::Parse::ParserBase parser("()", "origin");
    auto value = rlisp::parse(parser, mempool);
    ASSERT_EQ(nullptr, parser.get_error());
    EXPECT_TRUE(value == mempool.intern_atom("nil"));
    EXPECT_EQ(mempool.num_roots(), 0);
}
TEST(Parser, SucceedOnAtom)
{
    rlisp::MemPool mempool;
    vcpkg::Parse::ParserBase parser("a", "origin");
    auto value = rlisp::parse(parser, mempool);
    ASSERT_EQ(nullptr, parser.get_error());
    EXPECT_TRUE(value == mempool.intern_atom("a"));
    EXPECT_EQ(mempool.num_roots(), 0);
}

TEST(Parser, SucceedOnLists)
{
    rlisp::MemPool mempool;
    Cons* list_cab;
    Cons* list_ab;
    Cons* list_b;
    {
        vcpkg::Parse::ParserBase parser("(b)", "origin");
        list_b = rlisp::parse(parser, mempool);
        ASSERT_EQ(parser.get_error(), nullptr);
        ASSERT_NE(list_b, nullptr);
        EXPECT_EQ(list_b->car, mempool.intern_atom("b"));
        EXPECT_EQ(list_b->cdr, mempool.intern_atom("nil"));
    }
    {
        vcpkg::Parse::ParserBase parser("(a b)", "origin");
        list_ab = rlisp::parse(parser, mempool);
        ASSERT_EQ(parser.get_error(), nullptr);
        ASSERT_NE(list_ab, nullptr);
        EXPECT_EQ(list_ab->car, mempool.intern_atom("a"));
        EXPECT_STRUCTURAL_EQ(list_ab->cdr, list_b);
    }
    {
        vcpkg::Parse::ParserBase parser("(c a b)", "origin");
        list_cab = rlisp::parse(parser, mempool);
        ASSERT_EQ(parser.get_error(), nullptr);
        ASSERT_NE(list_cab, nullptr);
        EXPECT_EQ(list_cab->car, mempool.intern_atom("c"));
        EXPECT_STRUCTURAL_EQ(list_cab->cdr, list_ab);
    }
    {
        vcpkg::Parse::ParserBase parser("(a . b)", "origin");
        list_cab = rlisp::parse(parser, mempool);
        ASSERT_EQ(parser.get_error(), nullptr);
        ASSERT_NE(list_cab, nullptr);
        EXPECT_EQ(list_cab->car, mempool.intern_atom("a"));
        EXPECT_EQ(list_cab->cdr, mempool.intern_atom("b"));
    }
    EXPECT_EQ(mempool.num_roots(), 0);
}
TEST(Parser, FailOnDot)
{
    rlisp::MemPool mempool;
    {
        vcpkg::Parse::ParserBase parser(".", "origin");
        rlisp::parse(parser, mempool);
        ASSERT_NE(parser.get_error(), nullptr);
    }

    {
        vcpkg::Parse::ParserBase parser(".a", "origin");
        rlisp::parse(parser, mempool);
        ASSERT_NE(parser.get_error(), nullptr);
    }

    {
        vcpkg::Parse::ParserBase parser("(. a)", "origin");
        rlisp::parse(parser, mempool);
        ASSERT_NE(parser.get_error(), nullptr);
    }

    {
        vcpkg::Parse::ParserBase parser("(a .)", "origin");
        rlisp::parse(parser, mempool);
        ASSERT_NE(parser.get_error(), nullptr);
    }

    {
        vcpkg::Parse::ParserBase parser("(a . )", "origin");
        rlisp::parse(parser, mempool);
        ASSERT_NE(parser.get_error(), nullptr);
    }

    {
        vcpkg::Parse::ParserBase parser("( . a)", "origin");
        rlisp::parse(parser, mempool);
        ASSERT_NE(parser.get_error(), nullptr);
    }
    EXPECT_EQ(mempool.num_roots(), 0);
}

TEST(Parser, SucceedOnNil)
{
    rlisp::MemPool mempool;
    vcpkg::Parse::ParserBase parser("nil", "origin");
    auto value = rlisp::parse(parser, mempool);
    ASSERT_EQ(nullptr, parser.get_error());
    EXPECT_EQ(value, mempool.intern_atom("nil"));
    EXPECT_EQ(mempool.num_roots(), 0);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
