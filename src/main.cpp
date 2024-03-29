#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include "AST.h"
#include "koopa.h"
#include "util.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[])
{
	// 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
	// compiler 模式 输入文件 -o 输出文件
	assert(argc == 5);
	auto mode = std::string(argv[1]);
	auto input = argv[2];
	auto output = argv[4];

	// 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
	yyin = fopen(input, "r");
	assert(yyin);
	// parse input file
	unique_ptr<BaseAST> ast;
	auto ret = yyparse(ast);
	assert(!ret);

	FILE *outfile = fopen(output, "w");

	

	// 输出IR
	// 将IR代码写入文件outfile
	if (mode == "-koopa")
	{
		// dump AST to IR file
		// 重定向
		ofstream to_IR(output);
		streambuf* cout_buf = cout.rdbuf();
		cout.rdbuf(to_IR.rdbuf());
		ast->Dump();
		cout.rdbuf(cout_buf);
	}
	// 输出汇编
	// 将汇编代码写入文件outfile
	// IR => string => raw program => visit函数写入汇编文件
	else if (mode == "-riscv")
	{
		// TODO: IR => string
		ostringstream to_str;
		streambuf* cout_buf = cout.rdbuf();
		cout.rdbuf(to_str.rdbuf());
		ast->Dump();
		cout.rdbuf(cout_buf);
		string IR = to_str.str();
	



		// below: string => raw => visit
		// 解析字符串 str, 得到 Koopa IR 程序
		koopa_program_t program;
		koopa_error_code_t ret = koopa_parse_from_string(IR.c_str(), &program);
		assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
		// 创建一个 raw program builder, 用来构建 raw program
		koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
		// 将 Koopa IR 程序转换为 raw program
		koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
		// 释放 Koopa IR 程序占用的内存
		koopa_delete_program(program);

		// 处理 raw program
		// ...
		// TODO:重定向到output
		ofstream to_riscv(output);
		cout_buf = cout.rdbuf();
		cout.rdbuf(to_riscv.rdbuf());
		// visit
		visit(raw);
		cout.rdbuf(cout_buf);

		// 处理完成, 释放 raw program builder 占用的内存
		// 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
		// 所以不要在 raw program 处理完毕之前释放 builder
		koopa_delete_raw_program_builder(builder);
	}

	fclose(outfile);
	return 0;
}
