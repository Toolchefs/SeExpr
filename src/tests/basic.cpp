/*
* Copyright Disney Enterprises, Inc.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License
* and the following modification to it: Section 6 Trademarks.
* deleted and replaced with:
*
* 6. Trademarks. This License does not grant permission to use the
* trade names, trademarks, service marks, or product names of the
* Licensor and its affiliates, except as required for reproducing
* the content of the NOTICE file.
*
* You may obtain a copy of the License at
* http://www.apache.org/licenses/LICENSE-2.0
*/

#include <gtest/gtest.h>

#include <SeExpr2/Expression.h>
#include <SeExpr2/ExprFunc.h>
#include <SeExpr2/Vec.h>
using namespace SeExpr2;

static int invocations = 0;
static double countInvocations(double x) {
    invocations++;
    return x;
}

struct Func : public ExprFuncSimple {
    Func() : ExprFuncSimple(true) {}

    virtual ExprType prep(ExprFuncNode* node, bool scalarWanted, ExprVarEnvBuilder& envBuilder) const {
        bool valid = true;
        valid &= node->checkArg(0, ExprType().FP(3).Constant(), envBuilder);
        valid &= node->checkArg(1, ExprType().String().Varying(), envBuilder);
        valid &= node->checkArg(2, ExprType().FP(2).Varying(), envBuilder);
        valid &= node->checkArg(3, ExprType().String().Constant(), envBuilder);
        return valid ? ExprType().FP(4) : ExprType().Error();
    }
    virtual ExprFuncNode::Data* evalConstant(const ExprFuncNode* node, ArgHandle args) const { return nullptr; }
    virtual void eval(ArgHandle args) {
        const char* s1 = args.inStr(1);
        const char* s2 = args.inStr(3);
        double sum1 = 0;
        for (const char* p = s1; *p != 0; p++) sum1 += *p;
        double sum2 = 0;
        for (const char* p = s2; *p != 0; p++) sum2 += *p;
        const double* foo = args.inFp(0);
        const double* bar = args.inFp(2);
        *(Vec4d*)args.outFpHandle() = Vec4d(foo[0] + foo[1] + foo[2], bar[0] + bar[1], sum1, sum2);
    }
} testFuncSimple;
ExprFunc testFunc(testFuncSimple, 4, 4);

struct SimpleExpression : public Expression {
    // Define simple scalar variable type that just stores the value it returns
    struct Var : public ExprVarRef {
        double value;
        Var() : ExprVarRef(ExprType().FP(1).Varying()) {}
        void eval(double* result) { result[0] = value; }
        void eval(const char**) {}
    };
    mutable Var x, y;

    // Custom variable resolver, only allow ones we specify
    ExprVarRef* resolveVar(const std::string& name) const {
        if (name == "x") return &x;
        if (name == "y") return &y;
        return 0;
    }

    // Make a custom sum function
    mutable ExprFunc customFunc;
    static double customFuncHelper(double x, double y) { return x + y; }
    mutable ExprFunc countInvocationsFunc;

    // Custom function resolver
    ExprFunc* resolveFunc(const std::string& name) const {
        if (name == "custom") return &customFunc;
        if (name == "testFunc") return &testFunc;
        if (name == "countInvocations") return &countInvocationsFunc;
        return 0;
    }

    // Constructor
    SimpleExpression(const std::string& str)
        : Expression(str), customFunc(customFuncHelper), countInvocationsFunc(countInvocations) {}
};

TEST(BasicTests, AddConstant) {
    Expression expr("8+4");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 12);
}

TEST(BasicTests, SubtractConstant) {
    Expression expr("8-4");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 4);
}

TEST(BasicTests, MultiplyConstant) {
    Expression expr("8*4");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 32);
}

TEST(BasicTests, DivideConstant) {
    Expression expr("8/4");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 2);
}

TEST(BasicTests, ModConstant) {
    Expression expr("8%4");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 0);
}

TEST(BasicTests, ExponentConstant) {
    Expression expr("3^2");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 9);
}

TEST(BasicTests, ParensConstant) {
    Expression expr("(3+4)");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 7);
}

TEST(BasicTests, VecValueConstant) {
    Expression expr("[7+4,7*4,9-4]");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    const double res[3] = {11,28,5};
    EXPECT_EQ(val[0], res[0]);
    EXPECT_EQ(val[1], res[1]);
    EXPECT_EQ(val[2], res[2]);
}

TEST(BasicTests, Vec) {
    Vec3d a(1, 2, 3), b(2, 3, 4);
    ASSERT_EQ(a.dot(b), 20);
    ASSERT_EQ(a.length2(), a.dot(a));
    Vec3d foo = copyRawVec3<double>(&b[0]);
    Vec3d* aRef = (Vec3d*)&b[0];
    ASSERT_EQ(foo, *aRef);
}

TEST(BasicTests, Variables) {
    SimpleExpression expr("$x+y");
    expr.x.value = 3;
    expr.y.value = 4;
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isConstant());
    EXPECT_TRUE(!expr.isVec());
    EXPECT_TRUE(expr.usesVar("x"));
    EXPECT_TRUE(expr.usesVar("y"));
    EXPECT_TRUE(!expr.usesVar("z"));
    const double* val = expr.evalFP();
    EXPECT_EQ(val[0], 7);
}

TEST(BasicTests, Custom) {
    SimpleExpression expr("custom(1,2)");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    EXPECT_TRUE(expr.isConstant());
    EXPECT_TRUE(expr.usesFunc("custom"));
    EXPECT_EQ(expr.evalFP()[0], 3);
}

TEST(BasicTests, Precedence) {
    SimpleExpression expr1("1+2*3");
    EXPECT_EQ(expr1.evalFP()[0], 7);
    SimpleExpression expr2("(1+2)*3");
    EXPECT_EQ(expr2.evalFP()[0], 9);
}

TEST(BasicTests, VectorAssignment) {
    SimpleExpression expr1("$foo=[0,1,2]; $foo=3; $foo");
    double val1 = expr1.evalFP()[0];
    EXPECT_EQ(val1, 3);

    SimpleExpression expr3("$foo=3; $foo=[0,1,2]; $foo");
    SimpleExpression expr4("[0,1,2]");
    Vec3d* val3 = (Vec3d*)expr3.evalFP();
    Vec3d* val4 = (Vec3d*)expr4.evalFP();
    EXPECT_EQ(*val3, *val4);
}

TEST(BasicTests, LogicalShortCircuiting) {
    auto testExpr = [&](const char* expr, int expectedOutput, int invocationsExpected) {
        SimpleExpression expr1(expr);
        if (!expr1.isValid()) throw std::runtime_error(expr1.parseError());
        invocations = 0;
        const double* val = expr1.evalFP();
        EXPECT_EQ(val[0], expectedOutput);
        EXPECT_EQ(invocations, invocationsExpected);
    };
    testExpr("countInvocations(1)&&countInvocations(0)", 0, 2);
    testExpr("countInvocations(1)&&countInvocations(1)", 1, 2);
    testExpr("countInvocations(0)&&countInvocations(1)", 0, 1);
    testExpr("countInvocations(0)&&countInvocations(0)", 0, 1);
    testExpr("countInvocations(1)||countInvocations(0)", 1, 1);
    testExpr("countInvocations(1)||countInvocations(1)", 1, 1);
    testExpr("countInvocations(0)||countInvocations(1)", 1, 2);
    testExpr("countInvocations(0)||countInvocations(0)", 0, 2);
    testExpr("1?countInvocations(5):countInvocations(10)", 5, 1);
    testExpr("0?countInvocations(5):countInvocations(10)", 10, 1);
    testExpr("countInvocations(0)||countInvocations(0)||countInvocations(0)", 0, 3);
}

TEST(BasicTests, IfThenElse) {
    auto doTest = [](
        const std::string& eStr, ExprType desiredType, bool shouldBeValid, std::function<void(const double*)> check) {
        SimpleExpression e(eStr);
        e.x.value = 0;
        if (Expression::debugging) {
            std::cerr << "---------------------------------------------------------" << std::endl;
            std::cerr << eStr << std::endl;
        }
        e.setDesiredReturnType(desiredType);
        bool valid = e.isValid();
        if (!valid) {
            if (Expression::debugging) {
                std::cerr << "***Failed expr***";
                std::cerr << e.parseError() << std::endl;
            }
            if (shouldBeValid) {
                return false;
            }
        } else if (valid) {
            const double* f = e.evalFP();
            check(f);
        }
        return true;
    };

    // Check that variables not assigned in both are eliminated!
    doTest("if(x){a=3;b=a;} else {c=[1,2,3];b=c;d=c;} b+[9,8,2]", TypeVec(3), false, [](const double* f) {
        Vec3d* ref = (Vec3d*)f;
        Vec3d val(10, 10, 5);
        EXPECT_EQ(val, *ref);
    });

    doTest("if(x){a=3;b=a;} else {c=[1,2,3];b=c;} c+[9,8,2]", TypeVec(3), false, [](const double*) {});

    // Check incompatible output types
    doTest("if(x){a=3;b=a;} else {c=[1,2,3];b=c;} c+[9,8,2]", TypeVec(2), false, [](const double*) {});

    // Check incompatible output types
    doTest("if(x){b=[1,2];} else {b=[1,2,3]} b[0]]", TypeVec(2), false, [](const double*) {});

    // Check same business but with empty if
    doTest("a=[1,2];if(x){} else {a=[1,2,3];} a[0]", TypeVec(3), false, [](const double*) {});

    // Check same business but with empty if
    doTest("a=[1,2];if(x){} else {a=[1,2,3];} a[0]", TypeVec(2), false, [](const double*) {});

    // Check same business but with empty if
    doTest("a=[1,2];if(x){} else {a=[1,2,3];} [5,6]", TypeVec(2), true, [](const double* f) {
        Vec2d* ref = (Vec2d*)f;
        Vec2d val(5, 6);
        EXPECT_EQ(val, *ref);
    });

    // Check same business but with empty if
    doTest(
        "a=[1,2];if(x){a=[1,2,3,4];}else{a=[4,3,2,1];} c0=a[0]+a[1]+a[2]+a[3];if(x){a=5;} else {a=[1,2,3];} "
        "[c0,a[0]+a[1]+a[2],0]",
        TypeVec(3),
        true,
        [](const double* f) {
            Vec3d* ref = (Vec3d*)f;
            Vec3d val(10, 6, 5);
            EXPECT_EQ(val, *ref);
        });

    // Check same business but with empty if
    doTest("if(x){a=3;b=a;} else {c=[1,2];b=c;} b+[9,9]", TypeVec(2), true, [](const double* f) {
        Vec2d* ref = (Vec2d*)f;
        Vec2d val(10, 11);
        EXPECT_EQ(val, *ref);
    });
}

TEST(BasicTests, NestedTernary) {
    SimpleExpression expr1("1?2:3?4:5");
    if (!expr1.isValid()) {
        throw std::runtime_error("parse error:\n" + expr1.parseError());
    }
    if (!expr1.isValid()) throw std::runtime_error(expr1.parseError());
    const double* val = expr1.evalFP();
    EXPECT_EQ(val[0], 2);
    // TODO: put this expr in foo=3?1:2;Cs*foo
}

template <int d>
const double* run(const std::string& a) {
    SimpleExpression e(a);
    e.setDesiredReturnType(TypeVec(d));
    if (!e.isValid()) throw std::runtime_error(e.parseError());
    return e.evalFP();
}

TEST(BasicTests, TestFunc) {
    EXPECT_EQ(*(const Vec4d*)run<4>("testFunc([33,44,55],\"a\",[22,33],\"b\")"),
              Vec4d(33 + 44 + 55, 22 + 33, int('a'), int('b')));  //,int('a'),int('b')));
    EXPECT_EQ(*(const Vec4d*)run<4>("testFunc(33,\"aa\",22,\"bc\")"), Vec4d(33 * 3, 22 * 2, 'a' + 'a', 'b' + 'c'));
}

TEST(BasicTests, GetVar) {
    EXPECT_EQ(*(const Vec3d*)run<3>("getVar(\"a\",[11,22,33])"), Vec3d(11, 22, 33));
    EXPECT_EQ(*(const Vec4d*)run<4>("a=[11,22,33,44];getVar(\"a\",[5,3,2])"), Vec4d(11, 22, 33, 44));
    EXPECT_EQ(*(const Vec3d*)run<3>("a=[11,22,33,44];getVar(\"aa\",[5,3,2])"), Vec3d(5, 3, 2));
    EXPECT_EQ(*(const Vec4d*)run<4>("[11,22,33,44]"), Vec4d(11, 22, 33, 44));
}

TEST(BasicTests, Modulo) {
    Expression expr("-0.3%2");
    EXPECT_TRUE(expr.isValid());
    EXPECT_TRUE(!expr.isVec());
    const double* val = expr.evalFP();
    EXPECT_TRUE(expr.isConstant());
    EXPECT_EQ(val[0], 1.7);
}
