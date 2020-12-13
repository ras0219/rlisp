#include <vcpkg/base/expected.h>
#include <vcpkg/base/files.h>
#include <vcpkg/base/parse.h>
#include <vcpkg/base/system.print.h>

#include <parser.h>

#include <argparse/argparse.hpp>

namespace System = vcpkg::System;

template<class Fn, class T = decltype((std::declval<Fn&>()()))>
vcpkg::ExpectedS<T> capture(Fn fn)
{
    try
    {
        return {fn(), vcpkg::expected_left_tag};
    }
    catch (const std::exception& err)
    {
        return {err.what(), vcpkg::expected_right_tag};
    }
    catch (...)
    {
        return {"unknown exception", vcpkg::expected_right_tag};
    }
}

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("rlisp");

    program.add_argument("file").help("parse and run file");

    auto parsed = capture([&] {
        program.parse_args(argc, argv);
        return 0;
    });

    if (!parsed)
    {
        System::print2(System::Color::error, "rlisp", parsed.error(), '\n');
        program.print_help();
        return 1;
    }

    auto file = program.get<std::string>("file");
    System::print2("Passed file ", file, "\n");

    auto& fs = vcpkg::Files::get_real_filesystem();
    auto contents = fs.read_contents(fs::u8path(file), VCPKG_LINE_INFO);

    vcpkg::Parse::ParserBase parser(contents, file);

    return 0;
}