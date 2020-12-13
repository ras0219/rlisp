// clang-format off
#include <system_error>
// clang-format on

#include <vcpkg/base/parse.h>

#include <parser.h>

using namespace rlisp;

Value* rlisp::parse(vcpkg::Parse::ParserBase& parser)
{
    parser.add_error("failed");

    return nullptr;
}