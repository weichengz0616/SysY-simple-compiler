#pragma once
#include <iostream>
#include <string>
#include <cassert>
#include <map>
#include <cmath>
#include <algorithm>
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
int32_t stack_frame_size = 0;            // (byte)
std::map<std::string, int32_t> arg_nums; // func name -> args num

// 某函数是否有返回值
std::map<std::string, bool> func_ret;
// 当前处理的函数名
std::string func_now;

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
int32_t visit(const koopa_raw_global_alloc_t &alloc, const int &type);
void visit(const koopa_raw_branch_t &branch);
void visit(const koopa_raw_jump_t &jump);
int32_t visit(const koopa_raw_call_t &call);
std::string visit(const koopa_raw_func_arg_ref_t &func_arg);

void getBlocksFrameSize(int32_t &size, int32_t &max_args, const koopa_raw_slice_t &bbs);
void getInstsFrameSize(int32_t &size, int32_t &max_args, const koopa_raw_slice_t &insts);

// 访问 raw program
void visit(const koopa_raw_program_t &program)
{
    // 执行一些其他的必要操作
    // ...
    // 添加运行时库
    func_ret["@getint"] = true;
    func_ret["@getch"] = true;
    func_ret["@getarray"] = true;
    func_ret["@putint"] = false;
    func_ret["@putch"] = false;
    func_ret["@putarray"] = false;
    func_ret["@starttime"] = false;
    func_ret["@stoptime"] = false;
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
            std::cout << std::endl;
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
    // 跳过函数声明
    if (func->bbs.len == 0)
        return;

    func_now = std::string(func->name);
    // 计算栈帧大小
    // 遍历函数的所有指令
    // S: 局部变量
    // R: call指令, 记录ra寄存器, 这里简单考虑--统统分配4字节
    // A: call对应的函数的参数个数

    // 需要记录该函数有无call
    // 需要记录每个函数的参数个数, 供之后的函数查询
    // 需要记录当前call的最大参数量
    int32_t local_var_size = 0;
    int32_t max_args = 0;
    getBlocksFrameSize(local_var_size, max_args, func->bbs);
    stack_frame_size = local_var_size + 4 + (std::max)(max_args - 8, 0) * 4;
    stack_frame_size = ((stack_frame_size - 1) / 16 + 1) * 16; // 对齐16bytes

    // 局部变量的起点:
    sp_offset = (std::max)(max_args - 8, 0) * 4;

    // 执行一些其他的必要操作
    // ...
    std::cout << "\t.text" << std::endl;
    std::cout << "\t.globl " << (func->name + 1) << std::endl; // +1是因为“@”
    std::cout << (func->name + 1) << ":" << std::endl;

    std::cout << "\tli t0, " << -stack_frame_size << std::endl;
    std::cout << "\tadd sp, sp, t0\n";
    std::cout << std::endl;

    // 保存ra
    if (stack_frame_size < 2048)
        std::cout << "\tsw ra, " << stack_frame_size - 4 << "(sp)\n";
    else
    {
        std::cout << "\tli t1, " << stack_frame_size - 4 << std::endl;
        std::cout << "\tadd t1, t1, sp\n";
        std::cout << "\tsw ra, 0(t1)\n";
    }
    // std::cout << "\tsw ra, 0(sp)\n";
    // sp_offset += 4;

    // 加载参数
    // !!!!不需要加载参数到栈上
    // for(int i = 0;i < func->params.len;i++)
    // {
    //     if(i < 8)
    //     {
    //         std::cout << "\tsw a" << i << ", " << i * 4 << "(sp)\n";
    //     }
    //     else
    //     {
    //         std::cout << "\tli t1, " << stack_frame_size + (i - 8) * 4 << std::endl;
    //         std::cout << "\tadd t1, t1, sp\n";
    //         std::cout << "\tlw t0, 0(t1)\n";
    //         std::cout << "\tsw t0, " << i * 4 << "(sp)\n";
    //     }
    // }

    // 访问所有基本块
    visit(func->bbs);
}

void getBlocksFrameSize(int32_t &size, int32_t &max_args, const koopa_raw_slice_t &bbs)
{
    assert(bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
    for (size_t i = 0; i < bbs.len; ++i)
    {
        auto ptr = bbs.buffer[i];
        getInstsFrameSize(size, max_args, reinterpret_cast<koopa_raw_basic_block_t>(ptr)->insts);
    }
}

void getInstsFrameSize(int32_t &size, int32_t &max_args, const koopa_raw_slice_t &insts)
{
    assert(insts.kind == KOOPA_RSIK_VALUE);
    for (size_t i = 0; i < insts.len; ++i)
    {
        auto ptr = reinterpret_cast<koopa_raw_value_t>(insts.buffer[i]);
        if (ptr->ty->tag == KOOPA_RTT_INT32)
            size += 4; // 4 bytes
        if (ptr->kind.tag == KOOPA_RVT_CALL)
        {
            max_args = (std::max)(max_args, int32_t(ptr->kind.data.call.args.len));
        }
    }
}

// 访问基本块
void visit(const koopa_raw_basic_block_t &bb)
{
    // 执行一些其他的必要操作
    // ...
    // 丑陋的写法
    if(std::string(bb->name + 1) != "entry")
        std::cout << bb->name + 1 << ":" << std::endl;
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
        // rv2reg[value] = visit(kind.data.binary);
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
        rv2offset[value] = visit(kind.data.global_alloc, 0);
        break;
    case KOOPA_RVT_BRANCH:
        visit(kind.data.branch);
        std::cout << std::endl;
        break;
    case KOOPA_RVT_JUMP:
        visit(kind.data.jump);
        std::cout << std::endl;
        break;
    case KOOPA_RVT_CALL:
        rv2offset[value] = visit(kind.data.call);
        std::cout << std::endl;
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        std::cout << "\t.data\n";
        std::cout << "\t.global " << value->name + 1 << std::endl;
        std::cout << value->name + 1 << ":\n";
        visit(kind.data.global_alloc, 1);
        break;
    default:
        // 其他类型暂时遇不到
        printf("raw value type: %d\n", kind.tag);
        assert(false);
    }
}

void visit(const koopa_raw_return_t &ret)
{
    koopa_raw_value_t ret_value = ret.value;
    if (ret_value)
    {
        func_ret[func_now] = true;
        if (ret_value->kind.tag == KOOPA_RVT_INTEGER)
        {
            std::cout << "\tli t0, " << ret_value->kind.data.integer.value << std::endl;
            std::cout << "\tmv a0, t0\n";
            // visit(ret_value);
        }
        else
        {
            // assert(ret_value->kind.tag == KOOPA_RVT_BINARY);
            if (ret_value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
            {
                // 读取的是全局变量
                std::cout << "\tla t0, " << ret_value->name + 1 << std::endl;
                std::cout << "\tlw a0, 0(t0)\n";
                return;
            }
            assert(rv2offset.find(ret_value) != rv2offset.end());
            int32_t offset = rv2offset[ret_value];
            if (offset < 2048)
                std::cout << "\tlw a0, " << offset << "(sp)\n";
            else
            {
                std::cout << "\tli t0, " << offset << std::endl;
                std::cout << "\tadd t0, t0, sp\n";
                std::cout << "\tlw a0, 0(t0)\n";
            }
        }

        // std::cout << "\tmv a0, " << rv2reg[ret_value] << std::endl;
    }
    else
    {
        func_ret[func_now] = false;
    }

    // 恢复ra
    if (stack_frame_size < 2048)
        std::cout << "\tlw ra, " << stack_frame_size - 4 << "(sp)\n";
    else
    {
        std::cout << "\tli t1, " << stack_frame_size - 4 << std::endl;
        std::cout << "\tadd t1, t1, sp\n";
        std::cout << "\tlw ra, 0(t1)\n";
    }
    // 恢复栈指针
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
    else if (binary.lhs->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        // 读取的是全局变量
        std::cout << "\tla t0, " << binary.lhs->name + 1 << std::endl;
        std::cout << "\tlw t0, 0(t0)\n";
        lhs_reg = "t0";
    }
    else
    {
        // lhs_reg = rv2reg[binary.lhs];

        int32_t offset = rv2offset[binary.lhs];
        if (offset < 2048)
            std::cout << "\tlw t0, " << offset << "(sp)\n";
        else
        {
            std::cout << "\tli t0, " << offset << std::endl;
            std::cout << "\tadd t0, t0, sp\n";
            std::cout << "\tlw t0, 0(t0)\n";
        }

        lhs_reg = "t0";
    }
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    {
        // visit(binary.rhs);
        // rhs_reg = rv2reg[binary.rhs];
        std::cout << "\tli t1, " << binary.rhs->kind.data.integer.value << std::endl;
        rhs_reg = "t1";
    }
    else if (binary.rhs->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        // 读取的是全局变量
        std::cout << "\tla t0, " << binary.rhs->name + 1 << std::endl;
        std::cout << "\tlw t1, 0(t0)\n";
        lhs_reg = "t1";
    }
    else
    {
        // rhs_reg = rv2reg[binary.rhs];
        int32_t offset = rv2offset[binary.rhs];
        if (offset < 2048)
            std::cout << "\tlw t1, " << offset << "(sp)\n";
        else
        {
            std::cout << "\tli t0, " << offset << std::endl;
            std::cout << "\tadd t0, t0, sp\n";
            std::cout << "\tlw t1, 0(t0)\n";
        }
        rhs_reg = "t1";
    }
    // reg = regs[reg_now % 14];
    // reg = rhs_reg;
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
    if (sp_offset < 2048)
        std::cout << "\tsw t0, " << sp_offset << "(sp)\n";
    else
    {
        std::cout << "\tli t1, " << sp_offset << std::endl;
        std::cout << "\tadd t1, t1, sp\n";
        std::cout << "\tsw t0, 0(t1)\n";
    }
    sp_offset += 4;
    return sp_offset - 4;
}

int32_t visit(const koopa_raw_load_t &load)
{
    if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        std::cout << "\tla t0, " << load.src->name + 1 << std::endl;
        std::cout << "\tlw t0, 0(t0)\n";
    }
    else
    {
        // 这里的src我猜应该是指向首条alloc指令??
        assert(rv2offset.find(load.src) != rv2offset.end());
        int32_t offset = rv2offset[load.src];
        if (offset < 2048)
            std::cout << "\tlw t0, " << offset << "(sp)\n";
        else
        {
            std::cout << "\tli t0, " << offset << std::endl;
            std::cout << "\tadd t0, t0, sp\n";
            std::cout << "\tlw t0, 0(t0)\n";
        }
    }

    if (sp_offset < 2048)
        std::cout << "\tsw t0, " << sp_offset << "(sp)\n";
    else
    {
        std::cout << "\tli t1, " << sp_offset << std::endl;
        std::cout << "\tadd t1, t1, sp\n";
        std::cout << "\tsw t0, 0(t1)\n";
    }
    sp_offset += 4;
    return sp_offset - 4;
}

void visit(const koopa_raw_store_t &store)
{
    // store语句只会出现在:
    // 1.变量初始化为数字面量/单个变量 => IR会直接替换成数值
    // 2.变量初始化为表达式
    // 3.函数参数
    // 因此全局变量不会出现在左侧
    // 但可能出现在右侧吗?????????
    // eg. var = 1; ===> 即对全局变量的赋值操作 是可能的 ====> 即store.dest

    // 现在正在处理的是全局变量使用.....store阶段.

    // std::cout << "==== store " << store.value->kind.tag << std::endl;
    if (store.value->kind.tag == KOOPA_RVT_INTEGER)
    {
        // store 一个数值
        std::cout << "\tli t0, " << store.value->kind.data.integer.value << std::endl;

        if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
        {
            std::cout << "\tla t1, " << store.dest->name + 1 << std::endl;
            std::cout << "\tsw t0, 0(t1)\n";

            return;
        }

        int32_t offset_dest = rv2offset[store.dest];
        if (offset_dest < 2048)
            std::cout << "\tsw t0, " << offset_dest << "(sp)\n";
        else
        {
            std::cout << "\tli t1, " << offset_dest << std::endl;
            std::cout << "\tadd t1, t1, sp\n";
            std::cout << "\tsw t0, 0(t1)\n";
        }
        return;
    }
    else if (store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF)
    {
        // 直接从函数参数处取
        std::string reg = visit(store.value->kind.data.func_arg_ref);

        if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
        {
            std::cout << "\tla t1, " << store.dest->name + 1 << std::endl;
            std::cout << "\tsw " << reg << ", 0(t1)\n";

            return;
        }

        int32_t offset_dest = rv2offset[store.dest];
        if (offset_dest < 2048)
            std::cout << "\tsw " << reg << ", " << offset_dest << "(sp)\n";
        else
        {
            std::cout << "\tli t1, " << offset_dest << std::endl;
            std::cout << "\tadd t1, t1, sp\n";
            std::cout << "\tsw " << reg << ", 0(t1)\n";
        }

        return;
    }

    // 从普通表达式取
    int32_t offset_value = rv2offset[store.value];
    if (offset_value < 2048)
        std::cout << "\tlw t0, " << offset_value << "(sp)\n";
    else
    {
        std::cout << "\tli t0, " << offset_value << std::endl;
        std::cout << "\tadd t0, t0, sp\n";
        std::cout << "\tlw t0, 0(t0)\n";
    }

    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        std::cout << "\tla t1, " << store.dest->name + 1 << std::endl;
        std::cout << "\tsw t0, 0(t1)\n";

        return;
    }

    int32_t offset_dest = rv2offset[store.dest];
    if (offset_dest < 2048)
        std::cout << "\tsw t0, " << offset_dest << "(sp)\n";
    else
    {
        std::cout << "\tli t1, " << offset_dest << std::endl;
        std::cout << "\tadd t1, t1, sp\n";
        std::cout << "\tsw t0, 0(t1)\n";
    }
}

int32_t visit(const koopa_raw_global_alloc_t &alloc, const int &type)
{
    // assert(sp_offset <= 2048);
    if (type == 0)
    {
        // 这里规定type==0表示局部变量alloc
        // 返回的是栈偏移
        sp_offset += 4;
        return sp_offset - 4;
    }
    else if (type == 1)
    {
        // type==1 表示全局变量alloc
        if (alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
        {
            std::cout << "\t.zero 4\n";
        }
        else if (alloc.init->kind.tag == KOOPA_RVT_INTEGER)
        {
            std::cout << "\t.word " << alloc.init->kind.data.integer.value << std::endl;
        }
        else
        {
            printf("global alloc init tag: %d\n", alloc.init->kind.tag);
            assert(false);
        }
        return -1;
    }
    else
    {
        assert(false);
    }
}

void visit(const koopa_raw_branch_t &branch)
{
    if (branch.cond->kind.tag == KOOPA_RVT_INTEGER)
    {
        std::cout << "\tli t0, " << branch.cond->kind.data.integer.value << std::endl;
    }
    else
    {
        assert(rv2offset.find(branch.cond) != rv2offset.end());
        int32_t offset = rv2offset[branch.cond];
        if (offset < 2048)
            std::cout << "\tlw t0, " << offset << "(sp)\n";
        else
        {
            std::cout << "\tli t0, " << offset << std::endl;
            std::cout << "\tadd t0, t0, sp\n";
            std::cout << "\tlw t0, 0(t0)\n";
        }
    }
    std::cout << "\tbnez t0, " << branch.true_bb->name + 1 << std::endl;
    std::cout << "\tj " << branch.false_bb->name + 1 << std::endl;
}

void visit(const koopa_raw_jump_t &jump)
{
    std::cout << "\tj " << jump.target->name + 1 << std::endl;
}

int32_t visit(const koopa_raw_call_t &call)
{
    for (size_t i = 0; i < call.args.len; i++)
    {
        if (i < 8)
        {
            std::cout << "\tli a" << i << ", "
                      << reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])->kind.data.integer.value
                      << std::endl;
        }
        else
        {
            std::cout << "\tli t0"
                      << reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i])->kind.data.integer.value
                      << std::endl;

            std::cout << "\tsw t0, " << (i - 8) * 4 << "(sp)\n";
            // sp_offset += 4;
        }
    }
    std::cout << "\tcall " << call.callee->name + 1 << std::endl;

    assert(func_ret.find(std::string(call.callee->name)) != func_ret.end());
    if (func_ret[std::string(call.callee->name)])
    {
        // std::cout << "call return\n";
        std::cout << "\tsw a0, " << sp_offset << "(sp)";

        sp_offset += 4;
        return sp_offset - 4;
    }
    else
    {
        // std::cout << "call no return\n";
    }

    return -1;
}

std::string visit(const koopa_raw_func_arg_ref_t &func_arg)
{
    // 标记第几个参数
    int index = func_arg.index;
    std::string reg;
    if (index < 8)
    {
        reg = "a" + std::to_string(index);
    }
    else
    {
        std::cout << "\tli t1, " << stack_frame_size + (index - 8) * 4 << std::endl;
        std::cout << "\tadd t1, t1, sp\n";
        std::cout << "\tlw t0, 0(t1)\n";
        reg = "t0";
    }

    return reg;
}
