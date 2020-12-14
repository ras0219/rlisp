#include "cons.h"
#include "eval.h"
#include "mempool.h"
#include "parser.h"
#include "testutil.h"
#include "vcpkgparser.h"
#include <gtest/gtest.h>

using namespace rlisp;

static void expect_eval(
    const char* e1, const char* e2, rlisp::MemPool& pool, const char* filename, unsigned long lineno)
{
    auto expect_eval_e1 = rlisp::parse(e1, pool);
    if (expect_eval_e1 == nullptr)
    {
        ADD_FAILURE_AT(filename, lineno) << "Parse of e1 failed";
        return;
    }
    auto expect_eval_e2 = rlisp::parse(e2, pool);
    if (expect_eval_e2 == nullptr)
    {
        ADD_FAILURE_AT(filename, lineno) << "Parse of e2 failed";
        return;
    }
    auto expect_eval_e3 = rlisp::eval(expect_eval_e1, pool);
    if (expect_eval_e3 == nullptr)
    {
        ADD_FAILURE_AT(filename, lineno) << "Eval of e1 failed";
        return;
    }
    expect_structural_eq(expect_eval_e3, expect_eval_e2, filename, lineno);
}

static void expect_eval_fail(const char* e1, rlisp::MemPool& pool, const char* filename, unsigned long lineno)
{
    auto expect_eval_e1 = rlisp::parse(e1, pool);
    if (expect_eval_e1 == nullptr)
    {
        ADD_FAILURE_AT(filename, lineno) << "Parse of e1 failed";
        return;
    }
    auto expect_eval_e3 = rlisp::eval(expect_eval_e1, pool);
    if (expect_eval_e3 != nullptr)
    {
        ADD_FAILURE_AT(filename, lineno) << "Eval of e1 succeded in error";
        return;
    }
}

#define EXPECT_EVAL(E1, E2, MEM) expect_eval((E1), (E2), (MEM), __FILE__, __LINE__)
#define EXPECT_EVAL_FAIL(E1, MEM) expect_eval_fail((E1), (MEM), __FILE__, __LINE__)

TEST(Eval, Eval)
{
    rlisp::MemPool mempool;
    EXPECT_EVAL("()", "()", mempool);
    EXPECT_EVAL("t", "t", mempool);
    EXPECT_EVAL_FAIL("abdf", mempool);
    EXPECT_EVAL_FAIL("(abdf)", mempool);

    EXPECT_EVAL("'a", "a", mempool);
    EXPECT_EVAL("''a", "'a", mempool);
    EXPECT_EVAL("''a", "(quote a)", mempool);
    EXPECT_EVAL("'()", "()", mempool);

    EXPECT_EVAL_FAIL("(cons 'a)", mempool);
    EXPECT_EVAL_FAIL("(cons 'a 'b 'c)", mempool);
    EXPECT_EVAL("(cons 'a 'b)", "(a . b)", mempool);
    EXPECT_EVAL("(cons 'a ())", "(a)", mempool);
    EXPECT_EVAL("(cons 'a (cons 'b ()))", "(a b)", mempool);

    EXPECT_EVAL("(car '(a . b))", "a", mempool);
    EXPECT_EVAL("(cdr '(a . b))", "b", mempool);
    EXPECT_EVAL_FAIL("(car)", mempool);
    EXPECT_EVAL_FAIL("(cdr)", mempool);
    EXPECT_EVAL_FAIL("(car 'a)", mempool);
    EXPECT_EVAL_FAIL("(cdr 'a)", mempool);
    EXPECT_EVAL_FAIL("(car '(a . b) nil)", mempool);
    EXPECT_EVAL_FAIL("(cdr '(a . b) nil)", mempool);

    EXPECT_EVAL("(eq 'a 'a)", "t", mempool);
    EXPECT_EVAL("(eq 'a 'b)", "nil", mempool);

    EXPECT_EVAL("(cond (t 'c))", "c", mempool);
    EXPECT_EVAL("(cond (nil 'b) (t 'c))", "c", mempool);
    EXPECT_EVAL("(cond ((eq 'a 'a) 'b) (t 'c))", "b", mempool);

    EXPECT_EQ(mempool.num_roots(), 0);
}

static Cons* parse_eval(const char* src, MemPool& pool)
{
    auto e = parse(src, pool);
    if (e != nullptr)
        return eval(e, pool);
    else
        return nullptr;
}

TEST(MemoryPool, GarbageCollect)
{
    rlisp::MemPool mempool(30);
    for (int x = 0; x < 30; ++x)
    {
        EXPECT_NE(parse_eval("'(a a a a a a a a a a a a a a a a a a)", mempool), nullptr);
    }
    EXPECT_EQ(mempool.num_roots(), 0);

    auto pinned = parse("(a b c (d e f))", mempool);
    mempool.push_root(pinned);

    for (int x = 0; x < 30; ++x)
    {
        EXPECT_NE(parse_eval("'(a a a a a a a a a a a a a a a a a a)", mempool), nullptr);
    }
    EXPECT_STRUCTURAL_EQ(pinned, parse("(a b c (d e f))", mempool));
    mempool.pop_root();
    EXPECT_EQ(mempool.num_roots(), 0);
}