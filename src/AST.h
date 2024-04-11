#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

// 全局变量
// 改成static能链接
// 咋不能全局??? 没道理啊
static int reg_cnt = 0;

// 所有 AST 的基类
class BaseAST
{
public:
	virtual ~BaseAST() = default;
	virtual std::string Dump() const = 0;
};

// ================================= lv1 & 2 ===========================
// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
	// 用智能指针管理对象
	std::unique_ptr<BaseAST> func_def;

	std::string Dump() const override
	{
		func_def->Dump();

		return std::string();
	}
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> func_type;
	std::string ident;
	std::unique_ptr<BaseAST> block;

	std::string Dump() const override
	{
		std::cout << "fun @" << ident << "(): ";
		func_type->Dump();
		block->Dump();

		return std::string();
	}
};

class FuncTypeAST : public BaseAST
{
public:
	std::string Dump() const override
	{
		std::cout << "i32\n";

		return std::string();
	}
};

class BlockAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> stmt;
	std::string Dump() const override
	{
		std::cout << "{\n";
		stmt->Dump();
		std::cout << "}\n";

		return std::string();
	}
};

class StmtAST : public BaseAST
{
public:
	// int number;
	std::unique_ptr<BaseAST> exp;

	std::string Dump() const override
	{
		std::cout << "%entry:\n";
		std::string exp_string = exp->Dump();
		std::cout << "\tret "
				  << exp_string;// 返回当前最新的reg
		std::cout << std::endl;

		return std::string();
	}
};

// class NumberAST : public BaseAST
// {
// public:
// 	int int_const;

// 	void Dump() const override
// 	{
// 	}
// };

// ================================= lv3 ===========================
class ExpAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> lor_exp;

	std::string Dump() const override
	{
		return lor_exp->Dump();

		return std::string();
	}
};

class PrimaryExpAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> exp;
	int32_t number;

	std::string Dump() const override
	{
		if (exp)
		{
			return exp->Dump();
		}
		else
		{
			// std::cout << " " << number << " ";

			return std::to_string(number);
		}

		return std::string();
	}
};

class UnaryExpAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> primary_exp;
	std::unique_ptr<BaseAST> unary_op;
	std::unique_ptr<BaseAST> unary_exp;

	std::string Dump() const override
	{
		if (primary_exp)
		{
			return primary_exp->Dump();
		}
		else if (unary_op && unary_exp)
		{

			std::string op = unary_op->Dump();
			std::string uexp = unary_exp->Dump();
			if (uexp[0] == '%')
			{
				if(op == "!")
				{
					std::cout << "\t%" << reg_cnt << " = eq " << uexp << ", 0" << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if(op == "-")
				{
					std::cout << "\t%" << reg_cnt << " = sub 0, " << uexp << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if(op == "+")
				{
					return uexp;
				}
				else
				{

				}
			}
			else
			{
				if(op == "!")
				{
					std::cout << "\t%" << reg_cnt << " = eq " << uexp << ", 0" << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if(op == "-")
				{
					std::cout << "\t%" << reg_cnt << " = sub 0, " << uexp << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if(op == "+")
				{
					return uexp;
				}
				else
				{
					
				}
			}
		}

		return std::string();
	}
};

class UnaryOpAST : public BaseAST
{
public:
	char op;

	std::string Dump() const override
	{
		switch (op)
		{
		case '!':
			return std::string("!");
			break;
		case '-':
			return std::string("-");
			break;
		case '+':
			return std::string("+");
			break;
		default:
			break;
		}
		//std::cout << std::endl;

		return std::string();
	}
};

class MulExpAST : public BaseAST
{
public:
	char op = 0;
	std::unique_ptr<BaseAST> unary_exp;
	std::unique_ptr<BaseAST> mul_exp;

	std::string Dump() const override
	{
		if(op)
		{
			std::string uexp = unary_exp->Dump();
			std::string mexp = mul_exp->Dump();

			if(op == '*')
				std::cout << "\t%" << reg_cnt << " = mul " << mexp << ", " << uexp << std::endl;
			else if(op == '/')
				std::cout << "\t%" << reg_cnt << " = div " << mexp << ", " << uexp << std::endl;
			else if(op == '%')
				std::cout << "\t%" << reg_cnt << " = mod " << mexp << ", " << uexp << std::endl;
			else
				std::cout << "mul exp error" << std::endl;
			
			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}
		else
		{
			return unary_exp->Dump();
		}

		return std::string();
	}
};

class AddExpAST : public BaseAST
{
public:
	char op = 0;
	std::unique_ptr<BaseAST> mul_exp;
	std::unique_ptr<BaseAST> add_exp;

	std::string Dump() const override
	{
		if(op)
		{
			std::string aexp = add_exp->Dump();
			std::string mexp = mul_exp->Dump();

			if(op == '+')
				std::cout << "\t%" << reg_cnt << " = add " << aexp << ", " << mexp << std::endl;
			else if(op == '-')
				std::cout << "\t%" << reg_cnt << " = sub " << aexp << ", " << mexp << std::endl;
			else
				std::cout << "add exp error" << std::endl;
			
			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}
		else
		{
			return mul_exp->Dump();
		}

		return std::string();
	}
};

class RelExpAST : public BaseAST
{
public:
	enum TYPE {ADD, L, G, LE, GE};
	//char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> rel_exp;
	std::unique_ptr<BaseAST> add_exp;

	std::string Dump() const override
	{
		if(type == ADD)
		{
			return add_exp->Dump();
		}
		else
		{
			std::string rel = rel_exp->Dump();
			std::string add = add_exp->Dump();

			if(type == L)
			{
				std::cout << "\t%" << reg_cnt << " = lt " << rel << ", " << add << std::endl;
			}
			else if(type == G)
			{
				std::cout << "\t%" << reg_cnt << " = gt " << rel << ", " << add << std::endl;
			}
			else if(type == LE)
			{
				std::cout << "\t%" << reg_cnt << " = le " << rel << ", " << add << std::endl;
			}
			else if(type == GE)
			{
				std::cout << "\t%" << reg_cnt << " = ge " << rel << ", " << add << std::endl;
			}
			else
				std::cout << "rel exp error" << std::endl;

			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}
		
		return std::string();
	}
};

class EqExpAST : public BaseAST
{
public:
	enum TYPE {REL, EQ, NEQ};
	//char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> eq_exp;
	std::unique_ptr<BaseAST> rel_exp;

	std::string Dump() const override
	{
		if(type == REL)
		{
			return rel_exp->Dump();
		}
		else
		{
			std::string eq = eq_exp->Dump();
			std::string rel = rel_exp->Dump();

			if(type == EQ)
			{
				std::cout << "\t%" << reg_cnt << " = eq " << eq << ", " << rel << std::endl;
			}
			else if(type == NEQ)
			{
				std::cout << "\t%" << reg_cnt << " = ne " << eq << ", " << rel << std::endl;
			}
			else
				std::cout << "eq exp error" << std::endl;

			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}
		
		return std::string();
	}
};

class LAndExpAST : public BaseAST
{
public:
	enum TYPE {EQ, LAND};
	//char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> land_exp;
	std::unique_ptr<BaseAST> eq_exp;

	std::string Dump() const override
	{
		if(type == EQ)
		{
			return eq_exp->Dump();
		}
		else
		{
			std::string land = land_exp->Dump();
			std::string eq = eq_exp->Dump();

			if(type == LAND)
			{
				std::cout << "\t%" << reg_cnt << " = ne " << land << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = ne " << eq << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = and " << "%" << reg_cnt - 1 << ", " << "%" << reg_cnt - 2 << std::endl;
			}
			else
				std::cout << "land exp error" << std::endl;

			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}
		
		return std::string();
	}
};

class LOrExpAST : public BaseAST
{
public:
	enum TYPE {LAND, LOR};
	//char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> lor_exp;
	std::unique_ptr<BaseAST> land_exp;

	std::string Dump() const override
	{
		if(type == LAND)
		{
			return land_exp->Dump();
		}
		else
		{
			std::string lor = lor_exp->Dump();
			std::string land = land_exp->Dump();

			if(type == LOR)
			{
				std::cout << "\t%" << reg_cnt << " = ne " << lor << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = ne " << land << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = or " << "%" << reg_cnt - 1 << ", " << "%" << reg_cnt - 2 << std::endl;
			}
			else
				std::cout << "lor exp error" << std::endl;

			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}
		
		return std::string();
	}
};

// ...
