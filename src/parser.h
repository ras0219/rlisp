#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace vcpkg::Parse
{
    struct ParserBase;
}

namespace rlisp
{
    struct Value
    {
        uint64_t m_field1;
        union
        {
            uint64_t m_u64;
            Value* m_ptr;
            char m_chardata[8];
            double m_f64;
        };
    };

    struct MemPool
    {
        Value m_values[100];
        int cur_value = 0;

        Value* alloc()
        {
            if (cur_value == sizeof(m_values) / sizeof(m_values[0])) abort();

            return &m_values[cur_value++];
        }
    };

    Value* parse(vcpkg::Parse::ParserBase& parser);
}
