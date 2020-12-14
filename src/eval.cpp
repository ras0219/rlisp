#include "eval.h"

#include "cons.h"
#include "mempool.h"

using namespace rlisp;

static Cons* single_arg(Cons* e, MemPool& pool)
{
    if (e->is_atom() || e->cdr != pool.nil()) return nullptr;
    return e->car;
}

static Cons* single_arg_eval(Cons* e, MemPool& pool)
{
    if (auto a = single_arg(e, pool)) return eval(a, pool);
    return nullptr;
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

// assume scope is pinned; e is not pinned
static Cons* eval2(Cons* e, Cons* scope, MemPool& pool)
{
    if (e == pool.nil()) return e;
    if (e->is_atom())
    {
        if (*e->atom == "t")
            return e;
        else
        {
            while (!scope->is_atom())
            {
                if (scope->car->is_atom()) return nullptr;
                if (scope->car->car == e) return scope->car->cdr;
                scope = scope->cdr;
            }
            return nullptr;
        }
    }

    ScopedPin pin(e, pool);

    if (e->car->is_atom())
    {
        auto& s = *e->car->atom;
        if (s == "cons" || s == "eq")
        {
            if (e->cdr->is_atom()) return nullptr;
            if (e->cdr->cdr->is_atom()) return nullptr;
            if (e->cdr->cdr->cdr != pool.nil()) return nullptr;
            auto a1 = eval2(e->cdr->car, scope, pool);
            pool.push_root(a1);
            auto a2 = eval2(e->cdr->cdr->car, scope, pool);
            pool.pop_root();
            if (s == "cons")
                return pool.alloc(a1, a2);
            else if (s == "eq")
                return a1 == a2 ? pool.intern_atom("t") : pool.nil();
        }
        else if (s == "lambda")
        {
            return pool.alloc(pool.intern_atom("closure"), e->cdr);
        }
        else if (s == "cond")
        {
            auto case_list = e->cdr;
            do
            {
                if (case_list->is_atom()) return nullptr;

                auto cur_case = case_list->car;
                case_list = case_list->cdr;

                if (cur_case->is_atom()) return nullptr;
                if (cur_case->cdr->is_atom()) return nullptr;
                if (cur_case->cdr->cdr != pool.nil()) return nullptr;

                auto ea = eval2(cur_case->car, scope, pool);
                if (ea == nullptr) return nullptr;
                if (ea == pool.nil()) continue;
                return eval2(cur_case->cdr->car, scope, pool);
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
                if (a->is_atom()) return nullptr;
                if (s == "car")
                    return a->car;
                else if (s == "cdr")
                    return a->cdr;
            }
        }
        return nullptr;
    }
    else
    {
        auto func = eval2(e->car, scope, pool);

        if (func->is_atom())
        {
            // atoms are not function objects
            return nullptr;
        }
        else if (func->car->is_atom("closure"))
        {
            // closure object:
            // (closure (x y) (+ x y))

            if (func->cdr->is_atom()) return nullptr;
            if (func->cdr->cdr->is_atom()) return nullptr;
            if (func->cdr->cdr->cdr != pool.nil()) return nullptr;
            auto arglist = func->cdr->car;
            auto expr = func->cdr->cdr->car;
            ScopedPin pin_func(func->cdr, pool);

            auto newscope = scope;
            auto applylist = e->cdr;
            ScopedPin pin_newscope(newscope, pool);
            do
            {
                if (applylist == pool.nil() && arglist == pool.nil())
                {
                    return eval2(expr, newscope, pool);
                }
                else if (applylist->is_atom() || arglist->is_atom())
                {
                    return nullptr;
                }
                else if (!arglist->car->is_atom())
                {
                    return nullptr;
                }
                else
                {
                    // bind
                    newscope = pool.alloc(pool.alloc(arglist->car, eval2(applylist->car, scope, pool)), newscope);
                    // replace pinned scope with newer scope
                    pool.pop_push_root(newscope);

                    applylist = applylist->cdr;
                    arglist = arglist->cdr;
                }
            } while (1);
        }
        return nullptr;
    }
}

Cons* rlisp::eval(Cons* e, MemPool& pool) { return eval2(e, pool.nil(), pool); }
