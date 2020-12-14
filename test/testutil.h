#pragma once

#include "cons.h"
#include <gtest/gtest.h>

static bool expect_structural_eq(rlisp::Cons* c1, rlisp::Cons* c2, const char* file, unsigned long lineno)
{
    if (c1->car == rlisp::atom_tag && c2->car == rlisp::atom_tag)
    {
        if (c1->atom != c2->atom)
        {
            ADD_FAILURE_AT(file, lineno) << "expected " << c1->atom << " == " << c2->atom;
        }
        return c1->cdr == c2->cdr;
    }
    else if (c1->car != rlisp::atom_tag && c2->car != rlisp::atom_tag)
    {
        return expect_structural_eq(c1->car, c2->car, file, lineno) &&
               expect_structural_eq(c1->car, c2->car, file, lineno);
    }
    else
    {
        ADD_FAILURE_AT(file, lineno) << "expected eq " << c1->car << " == " << c2->car;
        return false;
    }
}

static void assert_structural_eq(rlisp::Cons* c1, rlisp::Cons* c2, const char* file, unsigned long lineno)
{
    ASSERT_TRUE(expect_structural_eq(c1, c2, file, lineno));
}

#define EXPECT_STRUCTURAL_EQ(C1, C2) (expect_structural_eq((C1), (C2), __FILE__, __LINE__))
#define ASSERT_STRUCTURAL_EQ(C1, C2) (assert_structural_eq((C1), (C2), __FILE__, __LINE__))
