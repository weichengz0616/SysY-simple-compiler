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
	virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
	// 用智能指针管理对象
	std::unique_ptr<BaseAST> func_def;

	void Dump() const override
	{
		func_def->Dump();
	}
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> func_type;
	std::string ident;
	std::unique_ptr<BaseAST> block;

	void Dump() const override
	{
		std::cout << "fun @" << ident << "(): ";
		func_type->Dump();
		block->Dump();
	}
};

class FuncTypeAST : public BaseAST
{
public:
	void Dump() const override
	{
		std::cout << "i32\n";
	}
};

class BlockAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> stmt;
	void Dump() const override
	{
		std::cout << "{\n";
		stmt->Dump();
		std::cout << "}\n";
	}
};

class StmtAST : public BaseAST
{
public:
	int number;

	void Dump() const override
	{
		std::cout << "%entry:\n";
		std::cout << "  ret " << number << std::endl;
	}
};

class NumberAST : public BaseAST
{
public:
	int int_const;

	void Dump() const override
	{
	}
};

// ...
