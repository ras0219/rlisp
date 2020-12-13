#include <parser.h>
#include <vcpkgparser.h>

#include <gtest/gtest.h>

namespace
{
    TEST(Parser, FailOnEmpty)
    {
        vcpkg::Parse::ParserBase parser("", "origin");
        auto value = rlisp::parse(parser);
        EXPECT_NE(nullptr, parser.get_error());
    }
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
