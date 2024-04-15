#pragma once
#include <iostream>
#include <string>
#include <cassert>
#include <map>
#include <cmath>
#include "koopa.h"

// std::string regs[14] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6",
//                         "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
// std::string zero_reg = "x0";
// static int reg_now = 0;

// 一条指令的结果 <=> 一个reg
// std::map<koopa_raw_value_t, std::string> rv2reg;

// 一条指令的结果 <=> 一个栈上的偏移量
std::map<koopa_raw_value_t, int32_t> rv2offset;
int32_t sp_offset = 0;
int32_t stack_frame_size = 0;// (byte)

// 函数声明略
// ...
void visit(const koopa_raw_program_t &program);
void visit(const koopa_raw_slice_t &slice);
void visit(const koopa_raw_function_t &func);
void visit(const koopa_raw_basic_block_t &bb);
void visit(const koopa_raw_value_t &value);
void visit(const koopa_raw_return_t &ret);
std::string visit(const koopa_raw_integer_t &integer);
int32_t visit(const koopa_raw_binary_t &binary);
int32_t visit(const koopa_raw_load_t &load);
void visit(const koopa_raw_store_t &store);
int32_t visit(const koopa_raw_global_alloc_t &alloc);

void getBlocksFrameSize(int32_t& size, const koopa_raw_slice_t& bbs);
void getInstsFrameSize(int32_t& size, const koopa_raw_slice_t& insts);

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
    // 计算栈帧大小
    // 遍历函数的所有指令
    
    getBlocksFrameSize(stack_frame_size, func->bbs);
    stack_frame_size = ((stack_frame_size - 1) / 16 + 1) * 16;// 对齐16bytes

    // 执行一些其他的必要操作
    // ...
    std::cout << "\t.text" << std::endl;
    std::cout << "\t.globl " << (func->name + 1) << std::endl; // +1是因为“@”
    std::cout << (func->name + 1) << ":" << std::endl;

    std::cout << "\tli t0, " << -stack_frame_size << std::endl;
    std::cout << "\tadd sp, sp, t0\n";
    std::cout << std::endl;
    // 访问所有基本块
    visit(func->bbs);
}

void getBlocksFrameSize(int32_t& size, const koopa_raw_slice_t& bbs)
{
    assert(bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
    for (size_t i = 0; i < bbs.len; ++i)
    {
        auto ptr = bbs.buffer[i];
        getInstsFrameSize(size, reinterpret_cast<koopa_raw_basic_block_t>(ptr)->insts);
    }
}

void getInstsFrameSize(int32_t& size, const koopa_raw_slice_t& insts)
{
    assert(insts.kind == KOOPA_RSIK_VALUE);
    for (size_t i = 0; i < insts.len; ++i)
    {
        auto ptr = reinterpret_cast<koopa_raw_value_t>(insts.buffer[i]);
        if(ptr->ty->tag == KOOPA_RTT_INT32)
            size += 4; // 4 bytes
    }
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
        std::cout << std::endl;
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        // rv2reg[value] = visit(kind.data.integer);
        // std::cout << std::endl;
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        //rv2reg[value] = visit(kind.data.binary);
        rv2offset[value] = visit(kind.data.binary);
        std::cout << std::endl;
        break;
    case KOOPA_RVT_LOAD:
        rv2offset[value] = visit(kind.data.load);
        std::cout << std::endl;
        break;
    case KOOPA_RVT_STORE:
        visit(kind.data.store);
        std::cout << std::endl;
        break;
    case KOOPA_RVT_ALLOC:
        rv2offset[value] = visit(kind.data.global_alloc);
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
            std::cout << "\tli t0, " << ret_value->kind.data.integer.value << std::endl;
            std::cout << "\tmv a0, t0\n";
            // visit(ret_value);
        }
        else
        {
            //assert(ret_value->kind.tag == KOOPA_RVT_BINARY);
            int32_t offset = rv2offset[ret_value];
            std::cout << "\tlw a0, " << offset << "(sp)\n";
        }

        // std::cout << "\tmv a0, " << rv2reg[ret_value] << std::endl;
        
    }

    std::cout << "\tli t0, " << stack_frame_size << std::endl;
    std::cout << "\tadd sp, sp, t0\n";
    std::cout << "\tret" << std::endl;
}

std::string visit(const koopa_raw_integer_t &integer)
{
    // std::string reg;
    // if (integer.value)
    // {
    //     reg = regs[reg_now % 14];
    //     std::cout << "\tli " << reg << ", " << integer.value << std::endl;
    //     reg_now++;
    // }
    // else
    //     reg = zero_reg;
    // return reg;
    return std::string();
}

int32_t visit(const koopa_raw_binary_t &binary)
{
    std::string lhs_reg, rhs_reg, reg;
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        // visit(binary.lhs);
        // lhs_reg = rv2reg[binary.lhs];
        std::cout << "\tli t0, " << binary.lhs->kind.data.integer.value << std::endl;
        lhs_reg = "t0";
    }
    else
    {
        // lhs_reg = rv2reg[binary.lhs];
        int32_t offset = rv2offset[binary.lhs];
        std::cout << "\tlw t0, " << offset << "(sp)\n";
        lhs_reg = "t0";
    }
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        // visit(binary.rhs);
        // rhs_reg = rv2reg[binary.rhs];
        std::cout << "\tli t1, " << binary.rhs->kind.data.integer.value << std::endl;
        rhs_reg = "t1";
    }
    else
    {
        // rhs_reg = rv2reg[binary.rhs];
        int32_t offset = rv2offset[binary.rhs];
        std::cout << "\tlw t1, " << offset << "(sp)\n";
        rhs_reg = "t1";
    }
    // reg = regs[reg_now % 14];
    //reg = rhs_reg;
    reg = "t0";

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

    // reg_now++;

    std::cout << "\tsw t0, " << sp_offset << "(sp)\n";
    sp_offset += 4;
    return sp_offset - 4;
}

int32_t visit(const koopa_raw_load_t &load)
{
    // 这里的src我猜应该是指向首条alloc指令??
    assert(rv2offset.find(load.src) != rv2offset.end());
    int32_t offset = rv2offset[load.src];
    std::cout << "\tlw t0, " << offset << "(sp)\n";

    std::cout << "\tsw t0, " << sp_offset << "(sp)\n";
    sp_offset += 4;
    return sp_offset - 4;
}

void visit(const koopa_raw_store_t &store)
{
    if(store.value->kind.tag == KOOPA_RVT_INTEGER)
    {
        int32_t offset_dest = rv2offset[store.dest];
        std::cout << "\tli t0, " << store.value->kind.data.integer.value << std::endl;
        std::cout << "\tsw t0, " << offset_dest << "(sp)\n";
        return;
    }
    int32_t offset_value = rv2offset[store.value];
    int32_t offset_dest = rv2offset[store.dest];
    std::cout << "\tlw t0, " << offset_value << "(sp)\n";
    std::cout << "\tsw t0, " << offset_dest << "(sp)\n";
}

int32_t visit(const koopa_raw_global_alloc_t &alloc)
{
    sp_offset += 4;
    return sp_offset - 4;
}
