#pragma once

#include <stdlib.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cons.h"

namespace rlisp
{
    struct MemPool
    {
        explicit MemPool(size_t sz = 512) : m_cells_size(sz), m_cells(new Cons[sz]) { }

        Cons* alloc(Cons* a, Cons* b);
        Cons* intern_atom(std::string atom);

        void push_root(Cons* a);
        void pop_root();
        void pop_push_root(Cons* a);

        Cons* nil();

        size_t num_roots() const { return m_roots.size(); }

    private:
        size_t m_cells_size;
        std::unique_ptr<Cons[]> m_cells;
        std::unordered_map<std::string, Cons> atoms;
        Cons m_nil{0, nullptr};
        std::string m_nil_str;
        int cur_value = 0;
        std::vector<Cons*> m_roots;
        Cons* m_free_list = nullptr;
    };
}
