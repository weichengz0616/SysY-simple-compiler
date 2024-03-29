#pragma once
// #include <cassert>
// #include <cstdio>
#include <iostream>
#include <memory>
#include <string>

// 所有 AST 的基类
class BaseAST
{
public:
	virtual ~BaseAST() = default;
	virtual void Dump(FILE *obj) const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
	// 用智能指针管理对象
	std::unique_ptr<BaseAST> func_def;

	void Dump(FILE *obj) const override
	{
		fprintf(obj, "\n");
		func_def->Dump(obj);
	}
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> func_type;
	std::string ident;
	std::unique_ptr<BaseAST> block;

	void Dump(FILE *obj) const override
	{
		fprintf(obj, "fun @%s(): ", ident.c_str());
		func_type->Dump(obj);
		block->Dump(obj);
	}
};

class FuncTypeAST : public BaseAST
{
public:
	void Dump(FILE *obj) const override
	{
		fprintf(obj, "i32\n");
	}
};

class BlockAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> stmt;
	void Dump(FILE *obj) const override
	{
		fprintf(obj, "{\n");
		stmt->Dump(obj);
		fprintf(obj, "}\n");
	}
};

class StmtAST : public BaseAST
{
public:
	int number;

	void Dump(FILE *obj) const override
	{
		fprintf(obj, "%%entry:\n");
		fprintf(obj, "  ret %d\n", number);
	}
};

class NumberAST : public BaseAST
{
public:
	int int_const;

	void Dump(FILE *obj) const override
	{
	}
};

// ...
