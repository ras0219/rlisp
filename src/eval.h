#pragma once

namespace rlisp
{
    struct Cons;
    struct MemPool;

    Cons* eval(Cons* expr, MemPool& pool);
}