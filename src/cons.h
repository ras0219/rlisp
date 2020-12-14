#pragma once

#include <string>

namespace rlisp
{
    struct Cons;

    using BuiltinFunc = Cons* (*)(Cons*, Cons*, struct MemPool&);

    struct Cons
    {
        bool is_atom() const { return car == 0; }
        bool is_atom(const char* v) const { return car == 0 && *atom == v; }
        bool is_cons() const { return 10 < (uintptr_t)car; }
        bool is_builtin() const { return 1 == (uintptr_t)car; }

        Cons* car;
        union
        {
            Cons* cdr;
            const std::string* atom;
            BuiltinFunc builtin;
        };
    };
}
