#pragma once

#include <string>

namespace rlisp
{
    struct Cons
    {
        Cons* car;
        union
        {
            Cons* cdr;
            const std::string* atom;
        };
    };

    static constexpr Cons* atom_tag{0};
}
