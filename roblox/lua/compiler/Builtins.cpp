// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Builtins.h"

#include "Bytecode.h"
#include "Compilerator.h"

#include "../../../security/xor.hpp"

namespace Compiler
{
namespace Compile
{

Builtin getBuiltin(AstExpr* node, const DenseHashMap<AstName, Global>& globals, const DenseHashMap<AstLocal*, Variable>& variables)
{
    if (AstExprLocal* expr = node->as<AstExprLocal>())
    {
        const Variable* v = variables.find(expr->local);

        return v && !v->written && v->init ? getBuiltin(v->init, globals, variables) : Builtin();
    }
    else if (AstExprIndexName* expr = node->as<AstExprIndexName>())
    {
        if (AstExprGlobal* object = expr->expr->as<AstExprGlobal>())
        {
            return getGlobalState(globals, object->name) == Global::Default ? Builtin{object->name, expr->index} : Builtin();
        }
        else
        {
            return Builtin();
        }
    }
    else if (AstExprGlobal* expr = node->as<AstExprGlobal>())
    {
        return getGlobalState(globals, expr->name) == Global::Default ? Builtin{AstName(), expr->name} : Builtin();
    }
    else
    {
        return Builtin();
    }
}

int getBuiltinFunctionId(const Builtin& builtin, const CompileOptions& options)
{
    if (builtin.empty())
        return -1;

    if (builtin.isGlobal(xor("assert")))
        return LBF_ASSERT;

    if (builtin.isGlobal(xor("type")))
        return LBF_TYPE;

    if (builtin.isGlobal(xor("typeof")))
        return LBF_TYPEOF;

    if (builtin.isGlobal(xor("rawset")))
        return LBF_RAWSET;
    if (builtin.isGlobal(xor("rawget")))
        return LBF_RAWGET;
    if (builtin.isGlobal(xor("rawequal")))
        return LBF_RAWEQUAL;

    if (builtin.isGlobal(xor("unpack")))
        return LBF_TABLE_UNPACK;

    if (builtin.object == xor("math"))
    {
        if (builtin.method == xor("abs"))
            return LBF_MATH_ABS;
        if (builtin.method == xor("acos"))
            return LBF_MATH_ACOS;
        if (builtin.method == xor("asin"))
            return LBF_MATH_ASIN;
        if (builtin.method == xor("atan2"))
            return LBF_MATH_ATAN2;
        if (builtin.method == xor("atan"))
            return LBF_MATH_ATAN;
        if (builtin.method == xor("ceil"))
            return LBF_MATH_CEIL;
        if (builtin.method == xor("cosh"))
            return LBF_MATH_COSH;
        if (builtin.method == xor("cos"))
            return LBF_MATH_COS;
        if (builtin.method == xor("deg"))
            return LBF_MATH_DEG;
        if (builtin.method == xor("exp"))
            return LBF_MATH_EXP;
        if (builtin.method == xor("floor"))
            return LBF_MATH_FLOOR;
        if (builtin.method == xor("fmod"))
            return LBF_MATH_FMOD;
        if (builtin.method == xor("frexp"))
            return LBF_MATH_FREXP;
        if (builtin.method == xor("ldexp"))
            return LBF_MATH_LDEXP;
        if (builtin.method == xor("log10"))
            return LBF_MATH_LOG10;
        if (builtin.method == xor("log"))
            return LBF_MATH_LOG;
        if (builtin.method == xor("max"))
            return LBF_MATH_MAX;
        if (builtin.method == xor("min"))
            return LBF_MATH_MIN;
        if (builtin.method == xor("modf"))
            return LBF_MATH_MODF;
        if (builtin.method == xor("pow"))
            return LBF_MATH_POW;
        if (builtin.method == xor("rad"))
            return LBF_MATH_RAD;
        if (builtin.method == xor("sinh"))
            return LBF_MATH_SINH;
        if (builtin.method == xor("sin"))
            return LBF_MATH_SIN;
        if (builtin.method == xor("sqrt"))
            return LBF_MATH_SQRT;
        if (builtin.method == xor("tanh"))
            return LBF_MATH_TANH;
        if (builtin.method == xor("tan"))
            return LBF_MATH_TAN;
        if (builtin.method == xor("clamp"))
            return LBF_MATH_CLAMP;
        if (builtin.method == xor("sign"))
            return LBF_MATH_SIGN;
        if (builtin.method == xor("round"))
            return LBF_MATH_ROUND;
    }

    if (builtin.object == xor("bit32"))
    {
        if (builtin.method == xor("arshift"))
            return LBF_BIT32_ARSHIFT;
        if (builtin.method == xor("band"))
            return LBF_BIT32_BAND;
        if (builtin.method == xor("bnot"))
            return LBF_BIT32_BNOT;
        if (builtin.method == xor("bor"))
            return LBF_BIT32_BOR;
        if (builtin.method == xor("bxor"))
            return LBF_BIT32_BXOR;
        if (builtin.method == xor("btest"))
            return LBF_BIT32_BTEST;
        if (builtin.method == xor("extract"))
            return LBF_BIT32_EXTRACT;
        if (builtin.method == xor("lrotate"))
            return LBF_BIT32_LROTATE;
        if (builtin.method == xor("lshift"))
            return LBF_BIT32_LSHIFT;
        if (builtin.method == xor("replace"))
            return LBF_BIT32_REPLACE;
        if (builtin.method == xor("rrotate"))
            return LBF_BIT32_RROTATE;
        if (builtin.method == xor("rshift"))
            return LBF_BIT32_RSHIFT;
        if (builtin.method == xor("countlz"))
            return LBF_BIT32_COUNTLZ;
        if (builtin.method == xor("countrz"))
            return LBF_BIT32_COUNTRZ;
    }

    if (builtin.object == xor("string"))
    {
        if (builtin.method == xor("byte"))
            return LBF_STRING_BYTE;
        if (builtin.method == xor("char"))
            return LBF_STRING_CHAR;
        if (builtin.method == xor("len"))
            return LBF_STRING_LEN;
        if (builtin.method == xor("sub"))
            return LBF_STRING_SUB;
    }

    if (builtin.object == xor("table"))
    {
        if (builtin.method == xor("insert"))
            return LBF_TABLE_INSERT;
        if (builtin.method == xor("unpack"))
            return LBF_TABLE_UNPACK;
    }

    if (options.vectorCtor)
    {
        if (options.vectorLib)
        {
            if (builtin.isMethod(options.vectorLib, options.vectorCtor))
                return LBF_VECTOR;
        }
        else
        {
            if (builtin.isGlobal(options.vectorCtor))
                return LBF_VECTOR;
        }
    }

    return -1;
}

} // namespace Compile
} // namespace Compiler
