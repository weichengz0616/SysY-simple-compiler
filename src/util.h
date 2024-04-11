#pragma once
#include <iostream>
#include <string>
#include <cassert>
#include <map>
#include <cmath>
#include "koopa.h"

std::string regs[14] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
                        "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
std::string zero_reg = "x0";
static int reg_now = 0;

// 一条指令的结果 <=> 一个reg
std::map<koopa_raw_value_t, std::string> rv2reg;

// 函数声明略
// ...
void visit(const koopa_raw_program_t &program);
void visit(const koopa_raw_slice_t &slice);
void visit(const koopa_raw_function_t &func);
void visit(const koopa_raw_basic_block_t &bb);
void visit(const koopa_raw_value_t &value);
void visit(const koopa_raw_return_t &ret);
std::string visit(const koopa_raw_integer_t &integer);
std::string visit(const koopa_raw_binary_t &binary);

// 访问 raw program
void visit(const koopa_raw_program_t &program)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有全局变量
    visit(program.values);
    // 访问所有函数
    visit(program.funcs);
}

// 访问 raw slice
void visit(const koopa_raw_slice_t &slice)
{
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问函数
void visit(const koopa_raw_function_t &func)
{
    // 执行一些其他的必要操作
    // ...
    std::cout << "\t.text" << std::endl;
    std::cout << "\t.globl " << (func->name + 1) << std::endl; // +1是因为“@”
    std::cout << (func->name + 1) << ":" << std::endl;
    // 访问所有基本块
    visit(func->bbs);
}

// 访问基本块
void visit(const koopa_raw_basic_block_t &bb)
{
    // 执行一些其他的必要操作
    // ...
    // std::cout << bb->name + 1 << ":" << std::endl;
    // 访问所有指令
    visit(bb->insts);
}

// 访问指令
void visit(const koopa_raw_value_t &value)
{
    // 根据指令类型
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        rv2reg[value] = visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 integer 指令
        rv2reg[value] = visit(kind.data.binary);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}

void visit(const koopa_raw_return_t &ret)
{
    koopa_raw_value_t ret_value = ret.value;
    if (ret_value)
    {
        if (ret_value->kind.tag == KOOPA_RVT_INTEGER)
        {
            visit(ret_value);
        }

        std::cout << "\tmv a0, " << rv2reg[ret_value] << std::endl;
    }

    std::cout << "\tret" << std::endl;
}

std::string visit(const koopa_raw_integer_t &integer)
{
    std::string reg;
    if (integer.value)
    {
        reg = regs[reg_now % 14];
        std::cout << "\tli " << reg << ", " << integer.value << std::endl;
        reg_now++;
    }
    else
        reg = zero_reg;
    return reg;
}

std::string visit(const koopa_raw_binary_t &binary)
{
    std::string lhs_reg, rhs_reg, reg;
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        visit(binary.lhs);
        lhs_reg = rv2reg[binary.lhs];
    }
    else
    {
        lhs_reg = rv2reg[binary.lhs];
    }
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        visit(binary.rhs);
        rhs_reg = rv2reg[binary.rhs];
    }
    else
    {
        rhs_reg = rv2reg[binary.rhs];
    }
    reg = regs[reg_now % 14];

    switch (binary.op)
    {
    case KOOPA_RBO_SUB:
    {
        std::cout << "\tsub " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    }
    case KOOPA_RBO_EQ:
    {
        std::cout << "\txor " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        std::cout << "\tseqz " << reg << ", " << reg << std::endl;
        break;
    }
    case KOOPA_RBO_ADD:
        std::cout << "\tadd " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    case KOOPA_RBO_MUL:
        std::cout << "\tmul " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    case KOOPA_RBO_DIV:
        std::cout << "\tdiv " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    case KOOPA_RBO_MOD:
        std::cout << "\trem " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    case KOOPA_RBO_NOT_EQ:
        std::cout << "\txor " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        std::cout << "\tsnez " << reg << ", " << reg << std::endl;
        break;
    case KOOPA_RBO_GT:
        std::cout << "\tslt " << reg << ", " << rhs_reg << ", " << lhs_reg << std::endl;
        break;
    case KOOPA_RBO_LT:
        std::cout << "\tslt " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    case KOOPA_RBO_GE:
        std::cout << "\tslt " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        std::cout << "\txori " << reg << ", " << reg << ", " << 1 << std::endl;
        break;
    case KOOPA_RBO_LE:
        std::cout << "\tslt " << reg << ", " << rhs_reg << ", " << lhs_reg << std::endl;
        std::cout << "\txori " << reg << ", " << reg << ", " << 1 << std::endl;
        break;
    case KOOPA_RBO_AND:
        std::cout << "\tand " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    case KOOPA_RBO_OR:
        std::cout << "\tor " << reg << ", " << lhs_reg << ", " << rhs_reg << std::endl;
        break;
    default:
        break;
    }

    reg_now++;
    return reg;
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...
