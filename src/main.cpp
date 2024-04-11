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

	// 输出IR
	// 将IR代码写入文件outfile
	if (mode == "-koopa")
	{
		// dump AST to IR file
		// 重定向
		ofstream to_IR(output);
		streambuf *cout_buf = cout.rdbuf();
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
		streambuf *cout_buf = cout.rdbuf();
		cout.rdbuf(to_str.rdbuf());
		ast->Dump();
		cout.rdbuf(cout_buf);
		string IR = to_str.str();

		// below: string => raw => visit 利用libkoopa
		koopa_program_t program;
		koopa_error_code_t ret = koopa_parse_from_string(IR.c_str(), &program);
		assert(ret == KOOPA_EC_SUCCESS); // 确保解析时没有出错
		koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
		koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
		koopa_delete_program(program);

		// 处理 raw program
		// 重定向到output输出文件
		ofstream to_riscv(output);
		cout_buf = cout.rdbuf();
		cout.rdbuf(to_riscv.rdbuf());
		// visit
		visit(raw);
		cout.rdbuf(cout_buf);

		// 处理完成, 释放 raw program builder
		koopa_delete_raw_program_builder(builder);
	}

	return 0;
}
