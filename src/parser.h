#pragma once

namespace vcpkg::Parse
{
    struct ParserBase;
}

namespace rlisp
{
    struct MemPool;
    struct Cons;

    Cons* parse(vcpkg::Parse::ParserBase& parser, MemPool& pool);
    Cons* parse(const char* data, const char* origin, MemPool& pool);
    Cons* parse(const char* data, MemPool& pool);
}
