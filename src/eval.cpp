#include "eval.h"

#include "cons.h"
#include "mempool.h"

using namespace rlisp;

static Cons* single_arg(Cons* e, MemPool& pool)
{
    if (!e->is_cons() || e->cdr != pool.nil()) return nullptr;
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

static Cons* eval2(Cons* e, Cons* scope, MemPool& pool);

static Cons* builtin_cond(Cons* e, Cons* scope, MemPool& pool)
{
    auto case_list = e->cdr;
    do
    {
        if (!case_list->is_cons()) return nullptr;

        auto cur_case = case_list->car;
        case_list = case_list->cdr;

        if (!cur_case->is_cons()) return nullptr;
        if (!cur_case->cdr->is_cons()) return nullptr;
        if (cur_case->cdr->cdr != pool.nil()) return nullptr;

        auto ea = eval2(cur_case->car, scope, pool);
        if (ea == nullptr) return nullptr;
        if (ea == pool.nil()) continue;
        return eval2(cur_case->cdr->car, scope, pool);
    } while (true);
}

static Cons* builtin_cons(Cons* e, Cons* scope, MemPool& pool)
{
    if (!e->cdr->is_cons()) return nullptr;
    if (!e->cdr->cdr->is_cons()) return nullptr;
    if (e->cdr->cdr->cdr != pool.nil()) return nullptr;
    auto a1 = eval2(e->cdr->car, scope, pool);
    if (a1 == nullptr) return nullptr;
    pool.push_root(a1);
    auto a2 = eval2(e->cdr->cdr->car, scope, pool);
    pool.pop_root();
    if (a2 == nullptr) return nullptr;
    return pool.alloc(a1, a2);
}

static Cons* builtin_eq(Cons* e, Cons* scope, MemPool& pool)
{
    if (!e->cdr->is_cons()) return nullptr;
    if (!e->cdr->cdr->is_cons()) return nullptr;
    if (e->cdr->cdr->cdr != pool.nil()) return nullptr;
    auto a1 = eval2(e->cdr->car, scope, pool);
    if (a1 == nullptr) return nullptr;
    pool.push_root(a1);
    auto a2 = eval2(e->cdr->cdr->car, scope, pool);
    pool.pop_root();
    if (a2 == nullptr) return nullptr;
    return a1 == a2 ? pool.intern_atom("t") : pool.nil();
}

static Cons* builtin_lambda(Cons* e, Cons*, MemPool& pool)
{
    auto closure = pool.intern_atom("closure");
    if (closure == nullptr) return nullptr;
    return pool.alloc(closure, e->cdr);
}

static Cons* builtin_quote(Cons* e, Cons*, MemPool& pool) { return single_arg(e->cdr, pool); }

static Cons* builtin_car(Cons* e, Cons* scope, MemPool& pool)
{
    if (auto a = single_arg_eval(e->cdr, pool))
    {
        if (a == pool.nil()) return a;
        if (!a->is_cons()) return nullptr;
        return a->car;
    }
    return nullptr;
}

static Cons* builtin_cdr(Cons* e, Cons* scope, MemPool& pool)
{
    if (auto a = single_arg_eval(e->cdr, pool))
    {
        if (a == pool.nil()) return a;
        if (!a->is_cons()) return nullptr;
        return a->cdr;
    }
    return nullptr;
}

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
            while (scope->is_cons())
            {
                if (!scope->car->is_cons()) return nullptr;
                if (scope->car->car == e) return scope->car->cdr;
                scope = scope->cdr;
            }
            return nullptr;
        }
    }
    else if (e->is_cons())
    {
        ScopedPin pin(e, pool);
        auto func = eval2(e->car, scope, pool);
        if (func == nullptr) return nullptr;

        if (func->is_builtin())
        {
            return func->builtin(e, scope, pool);
        }
        else if (!func->is_cons())
        {
            // only cons's can be function objects
            return nullptr;
        }
        else if (func->car->is_atom("closure"))
        {
            // closure object:
            // (closure (x y) (+ x y))

            if (!func->cdr->is_cons()) return nullptr;
            if (!func->cdr->cdr->is_cons()) return nullptr;
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
                else if (!applylist->is_cons() || !arglist->is_cons())
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
                    auto ea = eval2(applylist->car, scope, pool);
                    if (ea == nullptr) return nullptr;
                    auto x = pool.alloc(arglist->car, ea);
                    if (x == nullptr) return nullptr;
                    newscope = pool.alloc(x, newscope);
                    if (newscope == nullptr) return nullptr;
                    // replace pinned scope with newer scope
                    pool.pop_push_root(newscope);

                    applylist = applylist->cdr;
                    arglist = arglist->cdr;
                }
            } while (1);
        }
        return nullptr;
    }
    else
        return nullptr;
}

struct BuiltinCons
{
    BuiltinCons(std::string&& name, BuiltinFunc func, Cons* prev_scope, MemPool& pool)
        : builtin{.car = (Cons*)1, .builtin = func}
        , scope_entry{pool.intern_atom(std::move(name)), &builtin}
        , scope{&scope_entry, prev_scope}
    {
    }

    Cons builtin;
    Cons scope_entry;
    Cons scope;
};

Cons* rlisp::eval(Cons* e, MemPool& pool)
{
    BuiltinCons l1("cond", &builtin_cond, pool.nil(), pool);
    BuiltinCons l2("lambda", &builtin_lambda, &l1.scope, pool);
    BuiltinCons l3("eq", &builtin_eq, &l2.scope, pool);
    BuiltinCons l4("cons", &builtin_cons, &l3.scope, pool);
    BuiltinCons l5("car", &builtin_car, &l4.scope, pool);
    BuiltinCons l6("cdr", &builtin_cdr, &l5.scope, pool);
    BuiltinCons l7("quote", &builtin_quote, &l6.scope, pool);
    return eval2(e, &l7.scope, pool);
}
