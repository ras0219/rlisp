#pragma once

#include <string>

namespace rlisp
{
    struct Cons
    {
        bool is_atom() const { return car == 0; }
        bool is_atom(const char* v) const { return car == 0 && *atom == v; }

        Cons* car;
        union
        {
            Cons* cdr;
            const std::string* atom;
        };
    };

    static constexpr Cons* atom_tag{0};
}
