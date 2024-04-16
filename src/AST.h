#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// 全局变量
// 改成static能链接
// 咋不能全局??? 没道理啊
static int reg_cnt = 0;

// 处理多个return语句, 只能return一次
// TODO: 此时只有main函数, 一个全局变量记录即可, 多个函数之后再处理.
static bool returned_flag = false;

// lv 4.1
// 常量求值
// 符号表
// lv 4.2 符号表也可以存变量

struct VALUE
{
	// tagged
	enum VALUE_TYPE
	{
		CONST,
		VAR
	};
	VALUE_TYPE tag;
	int32_t value;
};
// static std::unordered_map<std::string, VALUE> symbol_table;

// lv5
// 作用域: 新进入一个block, 就新建一个符号表接在链表后面
struct SYMBOL_TABLE
{
	std::unordered_map<std::string, VALUE> table;
	SYMBOL_TABLE *prev;
	SYMBOL_TABLE *next;
	int tag;

	SYMBOL_TABLE()
	{
		prev = nullptr;
		next = nullptr;
		tag = 0;
		table = std::unordered_map<std::string, VALUE>();
	}
};
static SYMBOL_TABLE *st_head;
static SYMBOL_TABLE *st_cur;

// 所有 AST 的基类
class BaseAST
{
public:
	virtual ~BaseAST() = default;
	virtual std::string Dump() const = 0;
	// 给各种表达式求值用
	// 非纯的虚函数必须有定义, 否则链接报错
	virtual int32_t getValue() const { return 0; }
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
		// std::cout << "compunit dump...\n";
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
		st_head = new SYMBOL_TABLE();
		st_cur = st_head;
		std::cout << "fun @" << ident << "(): ";
		func_type->Dump();
		std::cout << "{\n";
		std::cout << "%entry:\n";
		block->Dump();
		std::cout << "}\n";

		delete st_head;
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
	std::vector<std::unique_ptr<BaseAST>> block_items;
	std::string Dump() const override
	{
		st_cur->next = new SYMBOL_TABLE();
		st_cur->next->prev = st_cur;
		st_cur->next->tag = st_cur->tag + 1;
		st_cur = st_cur->next;

		
		// stmt->Dump();
		for (auto i = block_items.begin(); i != block_items.end(); i++)
		{
			// std::cout << "blockitem dump...\n";
			//  if((*i))
			//  	std::cout << "nullptr...\n";

			(*i)->Dump();
		}
		

		st_cur = st_cur->prev;
		delete st_cur->next;
		st_cur->next = nullptr;

		return std::string();
	}
};

class StmtAST : public BaseAST
{
public:
	// int number;
	enum TYPE
	{
		LVAL,
		ZERO_RETURN,
		ONE_RETURN,
		ZERO_EXP,
		ONE_EXP,
		BLOCK
	};
	TYPE type;
	std::unique_ptr<BaseAST> lval;
	std::unique_ptr<BaseAST> exp;
	std::unique_ptr<BaseAST> block;

	std::string Dump() const override
	{
		if (type == ONE_RETURN)
		{
			returned_flag = true;
			// std::cout << "stmt dump...return\n";
			std::string exp_string = exp->Dump();
			std::cout << "\tret "
					  << exp_string; // 返回当前最新的reg
			std::cout << std::endl;
		}
		else if(type == ZERO_RETURN)
		{
			returned_flag = true;
			std::cout << "\tret\n";
		}
		else if (type == LVAL)
		{
			// std::cout << "stmt dump...lval\n";
			std::string exp_string = exp->Dump();
			// 此时lval必定是变量, 否则是语义错误
			std::string lval_string = lval->Dump();
			int32_t new_value = exp->getValue();

			auto st_tmp = st_cur;
			while (st_tmp->table.find(lval_string) == st_tmp->table.end())
			{
				st_tmp = st_tmp->prev;
			}
			auto &symbol_table = st_tmp->table;
			assert(symbol_table.find(lval_string) != symbol_table.end());
			symbol_table[lval_string].value = new_value;
			std::cout << "\tstore " << exp_string << " , @" << lval_string << "_" << st_tmp->tag << std::endl;
		}
		else if(type == ZERO_EXP)
		{
			// 啥也不干
		}
		else if(type == ONE_EXP)
		{
			exp->Dump();
		}
		else if(type == BLOCK)
		{
			block->Dump();
		}
		else
			std::cout << "stmt type error...\n";

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

	int32_t getValue() const override
	{
		return lor_exp->getValue();
	}
};

class PrimaryExpAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> exp;
	int32_t number;
	std::unique_ptr<BaseAST> lval;

	std::string Dump() const override
	{
		if (exp)
		{
			return exp->Dump();
		}
		else if (lval)
		{
			// std::cout << " " << number << " ";
			std::string ident = lval->Dump();
			auto st_tmp = st_cur;
			while (st_tmp->table.find(ident) == st_tmp->table.end())
			{
				st_tmp = st_tmp->prev;
			}
			auto &symbol_table = st_tmp->table;
			if (symbol_table[ident].tag == VALUE::CONST)
				return std::to_string(lval->getValue());
			else
			{
				std::cout << "\t%" << reg_cnt << " = load @" << ident << "_" << st_tmp->tag << std::endl;
				reg_cnt++;
				return "%" + std::to_string(reg_cnt - 1);
			}
		}
		else
			return std::to_string(number);

		return std::string();
	}

	int32_t getValue() const override
	{
		if (exp)
		{
			return exp->getValue();
		}
		else if (lval)
		{
			return lval->getValue();
		}
		else
		{
			return number;
		}
		return 0;
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
				if (op == "!")
				{
					std::cout << "\t%" << reg_cnt << " = eq " << uexp << ", 0" << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if (op == "-")
				{
					std::cout << "\t%" << reg_cnt << " = sub 0, " << uexp << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if (op == "+")
				{
					return uexp;
				}
				else
				{
				}
			}
			else
			{
				if (op == "!")
				{
					std::cout << "\t%" << reg_cnt << " = eq " << uexp << ", 0" << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if (op == "-")
				{
					std::cout << "\t%" << reg_cnt << " = sub 0, " << uexp << std::endl;
					reg_cnt++;

					return "%" + std::to_string(reg_cnt - 1);
				}
				else if (op == "+")
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

	int32_t getValue() const override
	{
		if (primary_exp)
		{
			return primary_exp->getValue();
		}
		else if (unary_op && unary_exp)
		{
			std::string op = unary_op->Dump();
			if (op == "!")
			{
				return 0 == unary_exp->getValue();
			}
			else if (op == "-")
			{
				return 0 - unary_exp->getValue();
			}
			else if (op == "+")
			{
				return unary_exp->getValue();
			}
		}
		return 0;
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
		// std::cout << std::endl;

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
		if (op)
		{
			std::string uexp = unary_exp->Dump();
			std::string mexp = mul_exp->Dump();

			if (op == '*')
				std::cout << "\t%" << reg_cnt << " = mul " << mexp << ", " << uexp << std::endl;
			else if (op == '/')
				std::cout << "\t%" << reg_cnt << " = div " << mexp << ", " << uexp << std::endl;
			else if (op == '%')
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

	int32_t getValue() const override
	{
		if (op)
		{
			if (op == '*')
				return mul_exp->getValue() * unary_exp->getValue();
			else if (op == '/')
				return mul_exp->getValue() / unary_exp->getValue();
			else if (op == '%')
				return mul_exp->getValue() % unary_exp->getValue();
		}
		else
		{
			return unary_exp->getValue();
		}
		return 0;
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
		if (op)
		{
			std::string aexp = add_exp->Dump();
			std::string mexp = mul_exp->Dump();

			if (op == '+')
				std::cout << "\t%" << reg_cnt << " = add " << aexp << ", " << mexp << std::endl;
			else if (op == '-')
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

	int32_t getValue() const override
	{
		if (op)
		{
			if (op == '+')
				return add_exp->getValue() + mul_exp->getValue();
			else if (op == '-')
				return add_exp->getValue() - mul_exp->getValue();
		}
		else
		{
			return mul_exp->getValue();
		}
		return 0;
	}
};

class RelExpAST : public BaseAST
{
public:
	enum TYPE
	{
		ADD,
		L,
		G,
		LE,
		GE
	};
	// char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> rel_exp;
	std::unique_ptr<BaseAST> add_exp;

	std::string Dump() const override
	{
		if (type == ADD)
		{
			return add_exp->Dump();
		}
		else
		{
			std::string rel = rel_exp->Dump();
			std::string add = add_exp->Dump();

			if (type == L)
			{
				std::cout << "\t%" << reg_cnt << " = lt " << rel << ", " << add << std::endl;
			}
			else if (type == G)
			{
				std::cout << "\t%" << reg_cnt << " = gt " << rel << ", " << add << std::endl;
			}
			else if (type == LE)
			{
				std::cout << "\t%" << reg_cnt << " = le " << rel << ", " << add << std::endl;
			}
			else if (type == GE)
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

	int32_t getValue() const override
	{
		if (type == ADD)
		{
			return add_exp->getValue();
		}
		else if (type == L)
		{
			return rel_exp->getValue() < add_exp->getValue();
		}
		else if (type == G)
		{
			return rel_exp->getValue() > add_exp->getValue();
		}
		else if (type == LE)
		{
			return rel_exp->getValue() <= add_exp->getValue();
		}
		else if (type == GE)
		{
			return rel_exp->getValue() >= add_exp->getValue();
		}
		return 0;
	}
};

class EqExpAST : public BaseAST
{
public:
	enum TYPE
	{
		REL,
		EQ,
		NEQ
	};
	// char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> eq_exp;
	std::unique_ptr<BaseAST> rel_exp;

	std::string Dump() const override
	{
		if (type == REL)
		{
			return rel_exp->Dump();
		}
		else
		{
			std::string eq = eq_exp->Dump();
			std::string rel = rel_exp->Dump();

			if (type == EQ)
			{
				std::cout << "\t%" << reg_cnt << " = eq " << eq << ", " << rel << std::endl;
			}
			else if (type == NEQ)
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

	int32_t getValue() const override
	{
		if (type == REL)
		{
			return rel_exp->getValue();
		}
		else if (type == EQ)
		{
			return rel_exp->getValue() == eq_exp->getValue();
		}
		else if (type == NEQ)
		{
			return rel_exp->getValue() != eq_exp->getValue();
		}
		return 0;
	}
};

class LAndExpAST : public BaseAST
{
public:
	enum TYPE
	{
		EQ,
		LAND
	};
	// char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> land_exp;
	std::unique_ptr<BaseAST> eq_exp;

	std::string Dump() const override
	{
		if (type == EQ)
		{
			return eq_exp->Dump();
		}
		else
		{
			std::string land = land_exp->Dump();
			std::string eq = eq_exp->Dump();

			if (type == LAND)
			{
				std::cout << "\t%" << reg_cnt << " = ne " << land << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = ne " << eq << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = and "
						  << "%" << reg_cnt - 1 << ", "
						  << "%" << reg_cnt - 2 << std::endl;
			}
			else
				std::cout << "land exp error" << std::endl;

			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}

		return std::string();
	}

	int32_t getValue() const override
	{
		if (type == EQ)
		{
			return eq_exp->getValue();
		}
		else if (type == LAND)
		{
			return land_exp->getValue() && eq_exp->getValue();
		}
		return 0;
	}
};

class LOrExpAST : public BaseAST
{
public:
	enum TYPE
	{
		LAND,
		LOR
	};
	// char op = 0;
	TYPE type;
	std::unique_ptr<BaseAST> lor_exp;
	std::unique_ptr<BaseAST> land_exp;

	std::string Dump() const override
	{
		if (type == LAND)
		{
			return land_exp->Dump();
		}
		else
		{
			std::string lor = lor_exp->Dump();
			std::string land = land_exp->Dump();

			if (type == LOR)
			{
				std::cout << "\t%" << reg_cnt << " = ne " << lor << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = ne " << land << ", " << 0 << std::endl;
				reg_cnt++;
				std::cout << "\t%" << reg_cnt << " = or "
						  << "%" << reg_cnt - 1 << ", "
						  << "%" << reg_cnt - 2 << std::endl;
			}
			else
				std::cout << "lor exp error" << std::endl;

			reg_cnt++;
			return "%" + std::to_string(reg_cnt - 1);
		}

		return std::string();
	}

	int32_t getValue() const override
	{
		if (type == LAND)
		{
			return land_exp->getValue();
		}
		else if (type == LOR)
		{
			return lor_exp->getValue() || land_exp->getValue();
		}
		return 0;
	}
};

// ================================= lv4 ===========================
class DeclAST : public BaseAST
{
public:
	enum TYPE
	{
		CONST,
		VAR
	};
	TYPE type;
	std::unique_ptr<BaseAST> const_decl;
	std::unique_ptr<BaseAST> var_decl;

	std::string Dump() const override
	{
		// 注意分类!!! 之前忘了, 导致空指针, 则段错误
		if (type == CONST)
			const_decl->Dump();
		else if (type == VAR)
			var_decl->Dump();
		return std::string();
	}
};

class ConstDeclAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> btype;
	std::vector<std::unique_ptr<BaseAST>> const_defs; // >= 1

	std::string Dump() const override
	{
		for (auto i = const_defs.begin(); i != const_defs.end(); i++)
		{
			(*i)->Dump();
		}
		return std::string();
	}
};

class BTypeAST : public BaseAST
{
public:
	std::string Dump() const override
	{

		return std::string();
	}
};

class ConstDefAST : public BaseAST
{
public:
	std::string ident;
	std::unique_ptr<BaseAST> const_init_val;

	std::string Dump() const override
	{
		updateSymbolTable();
		return std::string();
	}

	void updateSymbolTable() const
	{
		VALUE tmp;
		tmp.tag = VALUE::CONST;
		tmp.value = const_init_val->getValue();
		auto& symbol_table = st_cur->table;
		symbol_table[ident] = tmp;
	}
};

class ConstInitValAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> const_exp;

	std::string Dump() const override
	{
		return std::string();
	}

	int32_t getValue() const override
	{
		return const_exp->getValue();
	}
};

class BlockItemAST : public BaseAST
{
public:
	enum TYPE
	{
		DECL,
		STMT
	};
	TYPE type;
	std::unique_ptr<BaseAST> decl;
	std::unique_ptr<BaseAST> stmt;

	std::string Dump() const override
	{
		if (returned_flag)
			return std::string();
		if (type == DECL)
		{
			decl->Dump();
		}
		else if (type == STMT)
		{
			stmt->Dump();
		}
		return std::string();
	}
};

class LValAST : public BaseAST
{
public:
	std::string ident;

	std::string Dump() const override
	{
		// 注意这里常量才会直接求值
		// 变量应该翻译成IR
		// assert(symbol_table.find(ident) != symbol_table.end());
		// if(symbol_table[ident].tag == VALUE::CONST)
		// 	return std::to_string(getValue());
		// else
		// 	return ident;
		// return std::string();
		return ident;
	}

	int32_t getValue() const override
	{
		auto st_tmp = st_cur;
		while (st_tmp->table.find(ident) == st_tmp->table.end())
		{
			st_tmp = st_tmp->prev;
		}
		auto &symbol_table = st_tmp->table;
		assert(symbol_table.find(ident) != symbol_table.end());
		return symbol_table[ident].value;
	}
};

class ConstExpAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> exp;

	std::string Dump() const override
	{
		return std::string();
	}

	int32_t getValue() const override
	{
		return exp->getValue();
	}
};

class VarDeclAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> btype;
	std::vector<std::unique_ptr<BaseAST>> var_defs;

	std::string Dump() const override
	{
		// std::cout << "var decl dump...\n";
		for (auto i = var_defs.begin(); i != var_defs.end(); i++)
		{
			(*i)->Dump();
		}
		return std::string();
	}
};

class VarDefAST : public BaseAST
{
public:
	enum TYPE
	{
		IDENT,
		INIT
	};
	TYPE type;
	std::string ident;
	std::unique_ptr<BaseAST> init_val;

	std::string Dump() const override
	{
		if (type == IDENT)
		{
			VALUE v;
			v.tag = VALUE::VAR;
			auto& symbol_table = st_cur->table;
			symbol_table[ident] = v;

			std::cout << "\t@" << ident << "_" << st_cur->tag << "= alloc i32\n";
		}
		else if (type == INIT)
		{
			VALUE v;
			v.tag = VALUE::VAR;
			v.value = init_val->getValue();
			auto& symbol_table = st_cur->table;
			symbol_table[ident] = v;

			// 初始化变量的时候, 若右边表达式含变量, 也是直接像常量一样求值吗, 还是当作表达式?
			// 这里先按常量求值处理
			std::cout << "\t@" << ident << "_" << st_cur->tag << "= alloc i32\n";
			std::cout << "\tstore " << v.value << " , @" << ident << "_" << st_cur->tag << std::endl;
		}

		return std::string();
	}
};

class InitValAST : public BaseAST
{
public:
	std::unique_ptr<BaseAST> exp;

	std::string Dump() const override
	{
		return std::string();
	}

	int32_t getValue() const override
	{
		return exp->getValue();
	}
};

// ...
