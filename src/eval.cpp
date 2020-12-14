#include "eval.h"

#include "cons.h"
#include "mempool.h"

using namespace rlisp;

static Cons* single_arg(Cons* e, MemPool& pool)
{
    if (e->car == atom_tag || e->cdr != pool.nil()) return nullptr;
    return e->car;
}

static Cons* single_arg_eval(Cons* e, MemPool& pool)
{
    if (auto a = single_arg(e, pool)) return eval(a, pool);
    return nullptr;
}

static Cons* rev_eval_n(Cons* e, int n, MemPool& pool)
{
    Cons* ret = pool.nil();
    while (n > 0)
    {
        if (e->car == atom_tag) return nullptr;
        ret = pool.alloc(eval(e->car, pool), ret);
        e = e->cdr;
        --n;
    }
    if (e != pool.nil()) return nullptr;
    return ret;
}

struct ScopedPin
{
    ScopedPin(Cons* c, MemPool& p) : pool(p) { pool.push_root(c); }
    ~ScopedPin() { pool.pop_root(); }

    ScopedPin(const ScopedPin&) = delete;
    ScopedPin& operator=(const ScopedPin&) = delete;
    ScopedPin(ScopedPin&&) = delete;
    ScopedPin& operator=(ScopedPin&&) = delete;

private:
    MemPool& pool;
};

Cons* rlisp::eval(Cons* e, MemPool& pool)
{
    if (e == pool.nil()) return e;
    if (e->car == atom_tag)
    {
        if (*e->atom == "t")
            return e;
        else
            return nullptr;
    }

    if (e->car->car == atom_tag)
    {
        ScopedPin pin(e, pool);

        auto& s = *e->car->atom;
        if (s == "cons" || s == "eq")
        {
            auto args = rev_eval_n(e->cdr, 2, pool);
            if (!args) return nullptr;
            auto a1 = args->cdr->car;
            auto a2 = args->car;
            if (s == "cons")
                return pool.alloc(a1, a2);
            else if (s == "eq")
                return a1 == a2 ? pool.intern_atom("t") : pool.nil();
        }
        else if (s == "cond")
        {
            auto case_list = e->cdr;
            do
            {
                if (case_list->car == atom_tag) return nullptr;

                auto cur_case = case_list->car;
                case_list = case_list->cdr;

                if (cur_case->car == atom_tag) return nullptr;
                if (cur_case->cdr->car == atom_tag) return nullptr;
                if (cur_case->cdr->cdr != pool.nil()) return nullptr;

                auto ea = eval(cur_case->car, pool);
                if (ea == nullptr) return nullptr;
                if (ea == pool.nil()) continue;
                return eval(cur_case->cdr->car, pool);
            } while (true);
        }
        else if (s == "quote")
        {
            return single_arg(e->cdr, pool);
        }
        else if (s == "car" || s == "cdr")
        {
            if (auto a = single_arg_eval(e->cdr, pool))
            {
                if (a == pool.nil()) return a;
                if (a->car == atom_tag) return nullptr;
                if (s == "car")
                    return a->car;
                else if (s == "cdr")
                    return a->cdr;
            }
        }
    }
    return nullptr;
}
