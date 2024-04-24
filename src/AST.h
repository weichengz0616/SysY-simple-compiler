#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <sstream>

// 全局变量
// 改成static能链接
// 咋不能全局??? 没道理啊
static uint32_t reg_cnt = 0;
// if-else cnt
static uint32_t if_cnt = 0;

// 处理多个return语句, 只能return一次
// TODO: 此时只有main函数, 一个全局变量记录即可, 多个函数之后再处理.
// static bool returned_flag = false;
// lv6: 考虑if-else语句
// 跟符号表类似, 每个block的作用域内能有一个return
// 懒得用双向链表了, 这个甚至不用往前查, 直接vector吧
static std::vector<bool> has_returned = std::vector<bool>();
static int rt_cur;

// lv 4.1
// 常量求值
// 符号表
// lv 4.2 符号表也可以存变量
// lv 8 也可以存函数名-----value = 0代表无返回值
struct VALUE
{
	// tagged
	enum VALUE_TYPE
	{
		CONST,
		VAR,
		FUNCTION,
		CONST_ARRAY,
		ARRAY
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
static SYMBOL_TABLE *st_head; // lv8 全局符号表
static SYMBOL_TABLE *st_cur;
static int st_tag_cnt = 0;

// lv7
// break/continue语句需要知道当前while循环的entry和end在哪里
// 这里用栈实现, 栈顶即当前
static std::stack<std::string> while_entry;
static std::stack<std::string> while_end;

class InitValAST;
class ConstInitValAST;

// 所有 AST 的基类
class BaseAST
{
public:
	virtual ~BaseAST() = default;
	virtual std::string Dump() const = 0;
	// 给各种表达式求值用
	// 非纯的虚函数必须有定义, 否则链接报错
	virtual int32_t getValue() const
	{
		assert(false);
		return 0;
	}

	// 求stmt的最后一句是不是return语句
	// virtual bool lastReturn() const { return false; }
};

// ================================= lv1 & 2 ===========================
// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
	// 用智能指针管理对象
	std::vector<std::unique_ptr<BaseAST>> global_defs;

	std::string Dump() const override
	{
		// std::cout << "compunit dump...\n";

		st_head = new SYMBOL_TABLE();
		st_cur = st_head;

		// 运行时库的声明
		std::cout << "decl @getint(): i32\n";
		std::cout << "decl @getch(): i32\n";
		std::cout << "decl @getarray(*i32): i32\n";
		std::cout << "decl @putint(i32)\n";
		std::cout << "decl @putch(i32)\n";
		std::cout << "decl @putarray(i32, *i32)\n";
		std::cout << "decl @starttime()\n";
		std::cout << "decl @stoptime()\n";
		std::cout << std::endl;
		// 将运行时库放进符号表
		VALUE v;
		v.tag = VALUE::FUNCTION;
		v.value = 1;
		st_head->table["getint"] = v;
		st_head->table["getch"] = v;
		st_head->table["getarray"] = v;
		v.value = 0;
		st_head->table["putint"] = v;
		st_head->table["putch"] = v;
		st_head->table["putarray"] = v;
		st_head->table["starttime"] = v;
		st_head->table["stoptime"] = v;

		for (auto &global_def : global_defs)
			global_def->Dump();

		delete st_head;
		return std::string();
	}
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
	enum TYPE
	{
		PARAMS,
		NO_PARAMS
	};
	TYPE type;
	std::unique_ptr<BaseAST> func_type;
	std::string ident;
	std::unique_ptr<BaseAST> block;
	std::vector<std::unique_ptr<BaseAST>> func_f_params;

	std::string Dump() const override
	{
		// std::cout << "====func duming...\n";
		//  st_head = new SYMBOL_TABLE();
		//  st_cur = st_head;
		VALUE v;
		v.tag = VALUE::FUNCTION;
		v.value = 0;
		assert(st_head->table.find(ident) == st_head->table.end());
		st_head->table[ident] = v;

		st_cur->next = new SYMBOL_TABLE();
		st_cur->next->prev = st_cur;
		st_cur->next->tag = st_tag_cnt + 1;
		st_tag_cnt++;
		st_cur = st_cur->next;

		has_returned.push_back(0);
		rt_cur = 0;
		;

		// std::cout << "funcdef: " << has_returned.size() << " " << rt_cur << std::endl;
		std::vector<std::string> params;
		std::cout << "fun @" << ident << "(";
		if (type == PARAMS)
		{
			for (int i = 0; i < func_f_params.size(); i++)
			{
				if (i == 0)
				{
					params.push_back(func_f_params[i]->Dump());
				}
				else
				{
					std::cout << ", ";
					params.push_back(func_f_params[i]->Dump());
				}
			}
		}
		std::cout << ")";
		std::string return_type = func_type->Dump();
		if (return_type == "int")
		{
			std::cout << ": i32\n";
			st_head->table[ident].value = 1;
		}
		else
		{
			std::cout << "\n";
			st_head->table[ident].value = 0;
		}
		std::cout << "{\n";
		std::cout << "%entry:\n";

		// 这里把所有参数都重新存起来, 简化后端实现
		// 不然在后端还得讨论IR 数值/表达式的值/参数值
		for (auto &ident : params)
		{
			// 加入符号表, 普通的局部变量
			VALUE v;
			v.tag = VALUE::VAR;
			auto &symbol_table = st_cur->table;
			symbol_table[ident] = v;

			std::cout << "\t@" << ident << "_" << st_cur->tag << "= alloc i32\n";
			std::cout << "\tstore "
					  << "%" << ident << "_" << st_cur->tag << " , @" << ident << "_" << st_cur->tag << std::endl;
		}

		block->Dump();
		if (!has_returned[rt_cur])
		{
			if (st_head->table[ident].value == 0)
			{
				std::cout << "\tret\n";
			}
			else
			{
				std::cout << "\tret 0\n";
			}
		}
		std::cout << "}\n\n";

		// delete st_head;
		st_cur = st_cur->prev;
		delete st_cur->next;
		st_cur->next = nullptr;

		has_returned.pop_back();

		return std::string();
	}
};

class TypeAST : public BaseAST
{
public:
	std::string type;

	std::string Dump() const override
	{
		return type;
	}
};

// class FuncTypeAST : public BaseAST
// {
// public:
// 	enum TYPE {INT, VOID};
// 	TYPE type;
// 	std::string Dump() const override
// 	{
// 		if(type == INT)
// 		{
// 			std::cout << ": i32\n";
// 			return "int";
// 		}
// 		else
// 		{
// 			std::cout << std::endl;
// 			return "void";
// 		}

// 		return std::string();
// 	}
// };

class FuncFParamAST : public BaseAST
{
public:
	std::string btype;
	std::string ident;

	std::string Dump() const override
	{
		std::cout << "%" << ident << "_" << st_cur->tag << ": " << btype;
		return ident;
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
		st_cur->next->tag = st_tag_cnt + 1;
		st_tag_cnt++;
		st_cur = st_cur->next;

		// has_returned.push_back(0);
		// rt_cur++;
		// std::cout << "block1: " << has_returned.size() << " " << rt_cur << std::endl;

		// stmt->Dump();
		// std::cout << "====block dumping...\n";
		for (auto i = block_items.begin(); i != block_items.end(); i++)
		{
			// std::cout << "blockitem dump...\n";
			//  if((*i))
			//  	std::cout << "nullptr...\n";
			// std::cout << "==== items size: " << block_items.size() << std::endl;
			(*i)->Dump();
		}

		st_cur = st_cur->prev;
		delete st_cur->next;
		st_cur->next = nullptr;

		// has_returned.pop_back();
		// rt_cur--;
		// std::cout << "block2: " << has_returned.size() << " " << rt_cur << std::endl;

		return std::string();
	}

	// bool lastReturn() const override
	// {
	// 	if(block_items.size() > 0)
	// 	{
	// 		return block_items[block_items.size() - 1]->lastReturn();
	// 	}
	// 	return false;
	// }
};

class StmtAST : public BaseAST
{
public:
	enum TYPE
	{
		MATCHED,
		OPEN
	};
	TYPE type;
	std::unique_ptr<BaseAST> matched_stmt;
	std::unique_ptr<BaseAST> open_stmt;

	std::string Dump() const override
	{
		// std::cout << "====stmt dumping...\n";
		if (type == MATCHED)
		{
			matched_stmt->Dump();
		}
		else if (type == OPEN)
		{
			open_stmt->Dump();
		}

		return std::string();
	}
};

// ================================= lv6 ===========================
class MatchedStmtAST : public BaseAST
{
public:
	enum TYPE
	{
		IFELSE,
		OTHER,
		WHILE
	};
	TYPE type;
	std::unique_ptr<BaseAST> exp;
	std::unique_ptr<BaseAST> matched_stmt1;
	std::unique_ptr<BaseAST> matched_stmt2;
	std::unique_ptr<BaseAST> other_stmt;

	std::string Dump() const override
	{
		// std::cout << "====match dumping... " << type << std::endl;
		if (type == IFELSE)
		{
			uint32_t now_if_cnt = if_cnt;
			if_cnt++;
			// std::cout << "================== matched ifelse dump...\n";
			std::string exp_string = exp->Dump();
			std::cout << "\tbr " << exp_string << ", %then_" << now_if_cnt << ", %else_" << now_if_cnt << std::endl;

			has_returned.push_back(0);
			rt_cur++;
			std::cout << "%then_" << now_if_cnt << ":\n";
			matched_stmt1->Dump();
			bool has_returned1 = has_returned[rt_cur];
			if (!has_returned1)
				std::cout << "\tjump %end_" << now_if_cnt << std::endl;
			has_returned.pop_back();
			rt_cur--;

			has_returned.push_back(0);
			rt_cur++;
			std::cout << "%else_" << now_if_cnt << ":\n";
			matched_stmt2->Dump();
			bool has_returned2 = has_returned[rt_cur];
			if (!has_returned2)
				std::cout << "\tjump %end_" << now_if_cnt << std::endl;
			has_returned.pop_back();
			rt_cur--;

			// fix bug lv6
			// 两个分支已经都有return了, 就不要后续stmt了
			if (has_returned1 && has_returned2)
			{
				has_returned[rt_cur] = true;
			}
			else
				std::cout << "%end_" << now_if_cnt << ":\n";
			// if_cnt++;
		}
		else if (type == OTHER)
		{
			// std::cout << "================== matched other dump...\n";
			other_stmt->Dump();
		}
		else if (type == WHILE)
		{
			uint32_t now_if_cnt = if_cnt;
			if_cnt++;
			while_entry.push("%while_entry_" + std::to_string(now_if_cnt));
			while_end.push("%while_end_" + std::to_string(now_if_cnt));

			std::cout << "\tjump %while_entry_" << now_if_cnt << std::endl;
			std::cout << "%while_entry_" << now_if_cnt << ":" << std::endl;

			std::string exp_string = exp->Dump();
			std::cout << "\tbr " << exp_string << ", %while_body_" << now_if_cnt << ", %while_end_" << now_if_cnt << std::endl;

			std::cout << "%while_body_" << now_if_cnt << ":" << std::endl;
			has_returned.push_back(0);
			rt_cur++;
			matched_stmt1->Dump();
			bool has_returned1 = has_returned[rt_cur];
			if (!has_returned1)
				std::cout << "\tjump %while_entry_" << now_if_cnt << std::endl;
			has_returned.pop_back();
			rt_cur--;

			while_entry.pop();
			while_end.pop();
			// if(has_returned1)
			// {
			// 	has_returned[rt_cur] = true;
			// }
			// else
			std::cout << "%while_end_" << now_if_cnt << ":\n";
		}

		return std::string();
	}
};

class OpenStmtAST : public BaseAST
{
public:
	enum TYPE
	{
		IF,
		IFELSE,
		WHILE
	};
	TYPE type;
	std::unique_ptr<BaseAST> exp;
	std::unique_ptr<BaseAST> stmt;
	std::unique_ptr<BaseAST> matched_stmt;
	std::unique_ptr<BaseAST> open_stmt;

	std::string Dump() const override
	{
		// std::cout << "====open dumping... " << type << std::endl;
		if (type == IF)
		{
			uint32_t now_if_cnt = if_cnt;
			if_cnt++;

			std::string exp_string = exp->Dump();
			std::cout << "\tbr " << exp_string << ", %then_" << now_if_cnt << ", %end_" << now_if_cnt << std::endl;

			has_returned.push_back(0);
			rt_cur++;
			std::cout << "%then_" << now_if_cnt << ":\n";
			stmt->Dump();
			if (!has_returned[rt_cur])
				std::cout << "\tjump %end_" << now_if_cnt << std::endl;
			has_returned.pop_back();
			rt_cur--;

			std::cout << "%end_" << now_if_cnt << ":\n";
			// if_cnt++;
		}
		else if (type == IFELSE)
		{
			uint32_t now_if_cnt = if_cnt;
			if_cnt++;

			std::string exp_string = exp->Dump();
			std::cout << "\tbr " << exp_string << ", %then_" << now_if_cnt << ", %else_" << now_if_cnt << std::endl;

			has_returned.push_back(0);
			rt_cur++;
			std::cout << "%then_" << now_if_cnt << ":\n";
			matched_stmt->Dump();
			if (!has_returned[rt_cur])
				std::cout << "\tjump %end_" << now_if_cnt << std::endl;
			has_returned.pop_back();
			rt_cur--;

			has_returned.push_back(0);
			rt_cur++;
			std::cout << "%else_" << now_if_cnt << ":\n";
			open_stmt->Dump();
			if (!has_returned[rt_cur])
				std::cout << "\tjump %end_" << now_if_cnt << std::endl;
			has_returned.pop_back();
			rt_cur--;

			std::cout << "%end_" << now_if_cnt << ":\n";
			// if_cnt++;
		}
		else if (type == WHILE)
		{
			uint32_t now_if_cnt = if_cnt;
			if_cnt++;
			while_entry.push("%while_entry_" + std::to_string(now_if_cnt));
			while_end.push("%while_end_" + std::to_string(now_if_cnt));

			std::cout << "\tjump %while_entry_" << now_if_cnt << std::endl;
			std::cout << "%while_entry_" << now_if_cnt << ":" << std::endl;

			std::string exp_string = exp->Dump();
			std::cout << "\tbr " << exp_string << ", %while_body_" << now_if_cnt << ", %while_end_" << now_if_cnt << std::endl;

			std::cout << "%while_body_" << now_if_cnt << ":" << std::endl;
			has_returned.push_back(0);
			rt_cur++;
			open_stmt->Dump();
			bool has_returned1 = has_returned[rt_cur];
			if (!has_returned1)
				std::cout << "\tjump %while_entry_" << now_if_cnt << std::endl;
			has_returned.pop_back();
			rt_cur--;

			while_entry.pop();
			while_end.pop();
			std::cout << "%while_end_" << now_if_cnt << ":\n";
		}
		else
		{
		}

		return std::string();
	}
};

class OtherStmtAST : public BaseAST
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
		BLOCK,
		BREAK,
		CONTINUE
	};
	TYPE type;
	std::unique_ptr<BaseAST> lval;
	std::unique_ptr<BaseAST> exp;
	std::unique_ptr<BaseAST> block;

	std::string Dump() const override
	{
		// std::cout << "====stmt others dumping...\n";
		if (type == ONE_RETURN)
		{
			// std::cout << "====stmt one return dumping...\n";
			has_returned[rt_cur] = true;
			// std::cout << "stmt dump...return\n";
			std::string exp_string = exp->Dump();
			std::cout << "\tret "
					  << exp_string; // 返回当前最新的reg
			std::cout << std::endl;
		}
		else if (type == ZERO_RETURN)
		{
			has_returned[rt_cur] = true;
			// 注意这里只能处理int函数
			std::cout << "\tret 0\n";
		}
		else if (type == LVAL)
		{
			// std::cout << "stmt dump...lval\n";
			std::string exp_string = exp->Dump();
			// 此时lval必定是变量, 否则是语义错误
			// lv9 lval还可能是数组变量, 此时应该写入地址
			std::string lval_string = lval->Dump();
			// int32_t new_value = exp->getValue();

			if (lval_string[0] == '%')
			{
				// 数组
				std::cout << "\tstore " << exp_string << " , " << lval_string << std::endl;
			}
			else
			{
				auto st_tmp = st_cur;
				while (st_tmp->table.find(lval_string) == st_tmp->table.end())
				{
					st_tmp = st_tmp->prev;
				}
				auto &symbol_table = st_tmp->table;
				assert(symbol_table.find(lval_string) != symbol_table.end());
				// symbol_table[lval_string].value = new_value;
				std::cout << "\tstore " << exp_string << " , @" << lval_string << "_" << st_tmp->tag << std::endl;
			}
		}
		else if (type == ZERO_EXP)
		{
			// 啥也不干
		}
		else if (type == ONE_EXP)
		{
			// std::cout << "====one exp dumping...\n";
			exp->Dump();
		}
		else if (type == BLOCK)
		{
			// std::cout << "================== other block dump...\n";
			block->Dump();
		}
		else if (type == BREAK)
		{
			std::cout << "\tjump " << while_end.top() << std::endl;
			std::cout << "%break_" << if_cnt << ":" << std::endl;
		}
		else if (type == CONTINUE)
		{
			std::cout << "\tjump " << while_entry.top() << std::endl;
			std::cout << "%continue_" << if_cnt << ":" << std::endl;
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
		// std::cout << "====primary exp dumping...\n";
		if (exp)
		{
			return exp->Dump();
		}
		else if (lval)
		{
			// std::cout << " " << number << " ";
			std::string ident = lval->Dump();
			if (ident[0] == '%')
			{
				std::cout << "\t%" << reg_cnt << " = load %" << reg_cnt - 1 << std::endl;
				reg_cnt++;
				return "%" + std::to_string(reg_cnt - 1);
			}

			auto st_tmp = st_cur;
			while (st_tmp->table.find(ident) == st_tmp->table.end())
			{
				st_tmp = st_tmp->prev;
			}
			assert(st_tmp);
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
	enum TYPE
	{
		IDENT,
		IDENT_PARAMS
	};
	TYPE type;
	std::unique_ptr<BaseAST> primary_exp;
	std::unique_ptr<BaseAST> unary_op;
	std::unique_ptr<BaseAST> unary_exp;
	std::string ident;
	std::vector<std::unique_ptr<BaseAST>> func_r_params;

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
		// function call
		else if (type == IDENT)
		{
			std::cout << "\t";
			// 有返回值
			if (st_head->table[ident].value == 1)
			{
				std::cout << "%" << reg_cnt << " = ";
				reg_cnt++;
			}

			std::cout << "call @" << ident;
			std::cout << "(";
			std::cout << ")\n";

			if (st_head->table[ident].value == 1)
			{
				return "%" + std::to_string(reg_cnt - 1);
			}
		}
		else if (type == IDENT_PARAMS)
		{
			std::vector<std::string> args;
			for (int i = 0; i < func_r_params.size(); i++)
			{
				args.push_back(func_r_params[i]->Dump());
			}

			std::cout << "\t";
			// 有返回值
			if (st_head->table[ident].value == 1)
			{
				std::cout << "%" << reg_cnt << " = ";
				reg_cnt++;
			}

			std::cout << "call @" << ident;
			std::cout << "(";

			// 参数
			for (int i = 0; i < args.size(); i++)
			{
				if (i == 0)
				{
					std::cout << args[i];
				}
				else
				{
					std::cout << ", " << args[i];
				}
			}

			std::cout << ")\n";

			if (st_head->table[ident].value == 1)
			{
				return "%" + std::to_string(reg_cnt - 1);
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
		else
		{
			printf("should call function...\n");
			assert(false);
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

// class BTypeAST : public BaseAST
// {
// public:
// 	std::string btype;
// 	std::string Dump() const override
// 	{
// 		return btype;
// 	}
// };

class ConstInitValAST : public BaseAST
{
public:
	enum TYPE
	{
		EXP,
		ZERO_ARRAY,
		ARRAY
	};
	TYPE type;
	std::unique_ptr<BaseAST> const_exp;
	std::vector<std::unique_ptr<BaseAST>> const_init_list;

	std::string Dump() const override
	{
		assert(false);
		return std::string();
	}

	int32_t getValue() const override
	{
		if (type == EXP)
			return const_exp->getValue();
		assert(false);
	}

	std::vector<int32_t> getValueVector(std::vector<int32_t> &widths)
	{
		// assert(type == ARRAY);

		// 处理初始化列表----------目标: 拍平成一个一维数组
		std::vector<int32_t> complete;
		int dim = widths.size();
		int all = 1;
		for (auto &iter : widths)
		{
			all *= iter;
		}

		if (type == ZERO_ARRAY)
		{
			complete = std::vector<int32_t>(all, 0);
			return complete;
		}
		else if (type == ARRAY)
		{
		}

		// 计数: 当前放进了多少个elem了
		int cnt = 0;
		for (auto &iter : const_init_list)
		{
			auto ptr = dynamic_cast<ConstInitValAST *>(iter.get());
			if (ptr->type == EXP)
			{
				cnt++;
				complete.emplace_back(ptr->getValue());
			}
			else
			{
				// array

				// 首先得看这个初始化列表占多少维
				// 注意这里没有考虑语义上的错误, 默认源代码语义正确.
				int tmp = 0;
				int num = cnt;
				if (num == 0)
				{
					tmp = dim - 1;
				}
				else
				{
					for (int i = dim - 1; i >= 0; i--)
					{
						tmp++;
						num /= widths[i];
						if (num == 1)
						{
							break;
						}
					}
				}

				std::vector<int32_t> sub_widths;
				for (int i = dim - tmp; i < dim; i++)
				{
					sub_widths.emplace_back(widths[i]);
				}
				auto sub_complete = ptr->getValueVector(sub_widths);
				cnt += sub_complete.size();
				complete.insert(complete.end(), sub_complete.begin(), sub_complete.end());
			}
		}

		for (int i = cnt; i < all; i++)
		{
			complete.emplace_back(0);
		}
		return complete;
	}
};

class ConstDefAST : public BaseAST
{
public:
	// 单变量 or 数组
	enum TYPE
	{
		ONE,
		ARRAY
	};
	TYPE type;
	std::string ident;
	std::unique_ptr<BaseAST> const_init_val;
	std::vector<std::unique_ptr<BaseAST>> const_exps;

	std::string Dump() const override
	{
		if (type == ONE)
		{
			VALUE tmp;
			tmp.tag = VALUE::CONST;
			tmp.value = const_init_val->getValue();
			auto &symbol_table = st_cur->table;
			symbol_table[ident] = tmp;
		}
		else if (type == ARRAY)
		{
			// 数组基本信息
			int32_t dim = const_exps.size();
			std::vector<int32_t> widths;
			std::vector<int32_t> suf_mul(dim, 0); // 后缀积
			for (auto &iter : const_exps)
			{
				widths.emplace_back(iter->getValue());
			}
			suf_mul[dim - 1] = 1;
			for (int i = dim - 2; i >= 0; i--)
			{
				suf_mul[i] = widths[i + 1] * suf_mul[i + 1];
			}

			// 生成数组type
			std::string array_type;
			for (int i = dim - 1; i >= 0; i--)
			{
				if (i == dim - 1)
				{
					array_type = "[i32, " + std::to_string(widths[i]) + "]";
				}
				else
				{
					array_type = "[" + array_type + ", " + std::to_string(widths[i]) + "]";
				}
			}

			// 处理初始化列表
			std::vector<int32_t> init_values = dynamic_cast<ConstInitValAST *>(const_init_val.get())->getValueVector(widths);

			// 处理符号表
			VALUE v;
			v.tag = VALUE::CONST_ARRAY;
			st_cur->table[ident] = v;

			// to IR
			if (st_cur == st_head)
			{
				std::string init_str = "{";
				for (int i = 0; i < init_values.size(); i++)
				{
					if (i == 0)
					{
						init_str += std::to_string(init_values[i]);
					}
					else
					{
						init_str += ", " + std::to_string(init_values[i]);
					}
				}
				init_str += "}";

				std::cout << "global @" << ident << "_" << st_cur->tag << " = alloc " << array_type
						  << ", " << init_str << std::endl;
				std::cout << std::endl;
			}
			else
			{

				std::cout << "\t@" << ident << "_" << st_cur->tag << " = alloc " << array_type << std::endl;

				// 利用getelemptr得到指针, 然后store初始化数据
				// dim个getelemptr, 本质是dim层循环
				for (int i = 0; i < init_values.size(); i++)
				{
					std::vector<int32_t> offsets(dim, 0);
					int tmp = i;
					for (int j = 0; j < dim; j++)
					{
						offsets[j] = tmp / suf_mul[j];
						tmp = tmp % suf_mul[j];

						if (j == 0)
						{
							std::cout << "\t%" << reg_cnt << " = getelemptr @" << ident << "_" << st_cur->tag << ", " << offsets[j] << std::endl;
							reg_cnt++;
						}
						else
						{
							std::cout << "\t%" << reg_cnt << " = getelemptr %" << reg_cnt - 1 << ", " << offsets[j] << std::endl;
							reg_cnt++;
						}
					}

					std::cout << "\tstore " << init_values[i] << ", %" << reg_cnt - 1 << std::endl;
				}
			}
		}

		return std::string();
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
		assert(rt_cur < has_returned.size());
		// std::cout << "blockitem dump..." << type << std::endl;
		if (has_returned[rt_cur])
			return std::string();
		if (type == DECL)
		{
			decl->Dump();
		}
		else if (type == STMT)
		{
			// std::cout << "blockitem stmt dump...\n";
			stmt->Dump();
		}
		return std::string();
	}

	// bool lastReturn() const override
	// {
	// 	if(type == DECL)
	// 	{
	// 		return decl->lastReturn();
	// 	}
	// 	else if(type == STMT)
	// 	{
	// 		return stmt->lastReturn();
	// 	}
	// }
};

class LValAST : public BaseAST
{
public:
	enum TYPE
	{
		IDENT,
		ARRAY
	};
	TYPE type;
	std::string ident;
	std::vector<std::unique_ptr<BaseAST>> exps;

	std::string Dump() const override
	{
		// lval 可能出现在两个地方, 这里的后续处理丢给后面的
		// 1.表达式中
		// 2.赋值语句左侧
		if (type == IDENT)
		{
			return ident;
		}
		else if (type == ARRAY)
		{
			// lv9.2
			// load多维数组
			// 数组基本信息
			int32_t dim = exps.size();
			std::vector<std::string> widths;
			for (auto &iter : exps)
			{
				widths.emplace_back(iter->Dump());
			}

			auto st_tmp = st_cur;
			while (st_tmp->table.find(ident) == st_tmp->table.end())
			{
				st_tmp = st_tmp->prev;
			}
			auto &symbol_table = st_tmp->table;
			assert(symbol_table.find(ident) != symbol_table.end());

			for (int j = 0; j < dim; j++)
			{
				if (j == 0)
				{
					std::cout << "\t%" << reg_cnt << " = getelemptr @" << ident << "_" << st_tmp->tag << ", " << widths[j] << std::endl;
					reg_cnt++;
				}
				else
				{
					std::cout << "\t%" << reg_cnt << " = getelemptr %" << reg_cnt - 1 << ", " << widths[j] << std::endl;
					reg_cnt++;
				}
			}

			return "%" + std::to_string(reg_cnt - 1);
		}

		assert(false);
		return ident;
	}

	int32_t getValue() const override
	{
		assert(type == IDENT);
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

class InitValAST : public BaseAST
{
public:
	enum TYPE
	{
		EXP,
		ZERO_ARRAY,
		ARRAY
	};
	TYPE type;
	std::unique_ptr<BaseAST> exp;
	std::vector<std::unique_ptr<BaseAST>> init_list;

	std::string Dump() const override
	{
		assert(type == EXP);
		return exp->Dump();
	}

	int32_t getValue() const override
	{
		assert(type == EXP);
		return exp->getValue();
	}

	// 保证能常量求值
	std::vector<int32_t> getValueVector(std::vector<int32_t> &widths)
	{
		// assert(type == ARRAY);
		// 处理初始化列表----------目标: 拍平成一个一维数组
		std::vector<int32_t> complete;
		int dim = widths.size();
		int all = 1;
		for (auto &iter : widths)
		{
			all *= iter;
		}

		if (type == ZERO_ARRAY)
		{
			complete = std::vector<int32_t>(all, 0);
			return complete;
		}
		else if (type == ARRAY)
		{
		}

		// 计数: 当前放进了多少个elem了
		int cnt = 0;
		for (auto &iter : init_list)
		{
			auto ptr = dynamic_cast<InitValAST *>(iter.get());
			if (ptr->type == EXP)
			{
				cnt++;
				complete.emplace_back(ptr->getValue());
			}
			else
			{
				// array

				// 首先得看这个初始化列表占多少维
				// 注意这里没有考虑语义上的错误, 默认源代码语义正确.
				int tmp = 0;
				int num = cnt;
				if (num == 0)
				{
					tmp = dim - 1;
				}
				else
				{
					for (int i = dim - 1; i >= 0; i--)
					{
						tmp++;
						num /= widths[i];
						if (num == 1)
						{
							break;
						}
					}
				}

				std::vector<int32_t> sub_widths;
				for (int i = dim - tmp; i < dim; i++)
				{
					sub_widths.emplace_back(widths[i]);
				}
				auto sub_complete = ptr->getValueVector(sub_widths);
				cnt += sub_complete.size();
				complete.insert(complete.end(), sub_complete.begin(), sub_complete.end());
			}
		}

		for (int i = cnt; i < all; i++)
		{
			complete.emplace_back(0);
		}
		return complete;
	}

	// 变量or表达式求值
	std::vector<std::string> getValueString(std::vector<int32_t> &widths)
	{
		std::vector<std::string> complete;
		int dim = widths.size();
		int all = 1;
		for (auto &iter : widths)
		{
			all *= iter;
		}

		if (type == ZERO_ARRAY)
		{
			complete = std::vector<std::string>(all, "0");
			return complete;
		}
		else if (type == ARRAY)
		{
		}

		// 计数: 当前放进了多少个elem了
		int cnt = 0;
		for (auto &iter : init_list)
		{
			auto ptr = dynamic_cast<InitValAST *>(iter.get());
			if (ptr->type == EXP)
			{
				cnt++;
				complete.emplace_back(ptr->Dump());
			}
			else
			{
				// array

				// 首先得看这个初始化列表占多少维
				// 注意这里没有考虑语义上的错误, 默认源代码语义正确.
				int tmp = 0;
				int num = cnt;
				if (num == 0)
				{
					tmp = dim - 1;
				}
				else
				{
					for (int i = dim - 1; i >= 0; i--)
					{
						tmp++;
						num /= widths[i];
						if (num == 1)
						{
							break;
						}
					}
				}

				std::vector<int32_t> sub_widths;
				for (int i = dim - tmp; i < dim; i++)
				{
					sub_widths.emplace_back(widths[i]);
				}
				auto sub_complete = ptr->getValueString(sub_widths);
				cnt += sub_complete.size();
				complete.insert(complete.end(), sub_complete.begin(), sub_complete.end());
			}
		}

		for (int i = cnt; i < all; i++)
		{
			complete.emplace_back("0");
		}
		return complete;
	}
};

class VarDefAST : public BaseAST
{
public:
	enum TYPE
	{
		IDENT,
		INIT,
		ARRAY,
		ARRAY_INIT
	};
	TYPE type;
	std::string ident;
	std::unique_ptr<BaseAST> init_val;
	std::vector<std::unique_ptr<BaseAST>> const_exps;

	std::string Dump() const override
	{
		if (type == IDENT)
		{
			if (st_cur == st_head)
			{
				// 全局变量
				VALUE v;
				v.tag = VALUE::VAR;
				auto &symbol_table = st_cur->table;
				symbol_table[ident] = v;

				std::cout << "global @" << ident << "_" << st_cur->tag << " = alloc i32, zeroinit\n";
				std::cout << "\n";
			}
			else
			{
				VALUE v;
				v.tag = VALUE::VAR;
				auto &symbol_table = st_cur->table;
				symbol_table[ident] = v;

				std::cout << "\t@" << ident << "_" << st_cur->tag << " = alloc i32\n";
			}
		}
		else if (type == INIT)
		{
			// std::cout << "var decl init dumping...\n";
			if (st_cur == st_head)
			{
				// 全局变量
				// ????? 问题是全局变量的声明, 能含 非常量 表达式吗
				VALUE v;
				v.tag = VALUE::VAR;
				v.value = init_val->getValue();
				auto &symbol_table = st_cur->table;
				symbol_table[ident] = v;

				std::cout << "global @" << ident << "_" << st_cur->tag << " = alloc i32, " << v.value << std::endl;
				// std::string init_value = init_val->Dump();
				// std::cout << "store " << v.value << " , @" << ident << "_" << st_cur->tag << std::endl;
				std::cout << "\n";
			}
			else
			{
				VALUE v;
				v.tag = VALUE::VAR;
				// v.value = init_val->getValue();
				auto &symbol_table = st_cur->table;
				symbol_table[ident] = v;

				// 初始化变量的时候, 若右边表达式含变量, 也是直接像常量一样求值吗, 还是当作表达式?
				// 这里先按常量求值处理
				// lv8 函数之后这里不能直接当常量处理, 而应该翻译成表达式!!!!
				// 注意了, SYSY中的所有常量decl时必须能够编译时求值, 变量的decl不用
				// 不能把含变量/函数调用的表达式赋值给常量
				// 那么全局变量呢???? 全局变量的声明初始值可以给诸如变量和函数表达式吗????
				std::cout << "\t@" << ident << "_" << st_cur->tag << " = alloc i32\n";
				std::string init_value = init_val->Dump();
				std::cout << "\tstore " << init_value << " , @" << ident << "_" << st_cur->tag << std::endl;
			}
		}
		else if (type == ARRAY)
		{
			// 数组基本信息
			int32_t dim = const_exps.size();
			std::vector<int32_t> widths;
			for (auto &iter : const_exps)
			{
				widths.emplace_back(iter->getValue());
			}

			// 生成数组type
			std::string array_type;
			for (int i = dim - 1; i >= 0; i--)
			{
				if (i == dim - 1)
				{
					array_type = "[i32, " + std::to_string(widths[i]) + "]";
				}
				else
				{
					array_type = "[" + array_type + ", " + std::to_string(widths[i]) + "]";
				}
			}

			// 处理符号表
			VALUE v;
			v.tag = VALUE::ARRAY;
			st_cur->table[ident] = v;

			if (st_cur == st_head)
			{
				std::cout << "global @" << ident << "_" << st_cur->tag << " = alloc " << array_type
						  << ", zeroinit" << std::endl;
				std::cout << std::endl;
			}
			else
			{

				std::cout << "\t@" << ident << "_" << st_cur->tag << " = alloc " << array_type << std::endl;
			}
		}
		else if (type == ARRAY_INIT)
		{
			// 数组基本信息
			int32_t dim = const_exps.size();
			std::vector<int32_t> widths;
			std::vector<int32_t> suf_mul(dim, 0); // 后缀积
			for (auto &iter : const_exps)
			{
				widths.emplace_back(iter->getValue());
			}
			suf_mul[dim - 1] = 1;
			for (int i = dim - 2; i >= 0; i--)
			{
				suf_mul[i] = widths[i + 1] * suf_mul[i + 1];
			}

			// 生成数组type
			std::string array_type;
			for (int i = dim - 1; i >= 0; i--)
			{
				if (i == dim - 1)
				{
					array_type = "[i32, " + std::to_string(widths[i]) + "]";
				}
				else
				{
					array_type = "[" + array_type + ", " + std::to_string(widths[i]) + "]";
				}
			}

			// 处理符号表
			VALUE v;
			v.tag = VALUE::CONST_ARRAY;
			st_cur->table[ident] = v;

			// to IR
			if (st_cur == st_head)
			{
				// 全局数组初始化
				// 初始化全是常量表达式, 应当直接求值
				std::vector<int32_t> init_values = dynamic_cast<InitValAST *>(init_val.get())->getValueVector(widths);
				std::string init_str = "{";
				for (int i = 0; i < init_values.size(); i++)
				{
					if (i == 0)
					{
						init_str += std::to_string(init_values[i]);
					}
					else
					{
						init_str += ", " + std::to_string(init_values[i]);
					}
				}
				init_str += "}";

				std::cout << "global @" << ident << "_" << st_cur->tag << " = alloc " << array_type
						  << ", " << init_str << std::endl;
				std::cout << std::endl;
			}
			else
			{
				std::vector<std::string> init_values = dynamic_cast<InitValAST *>(init_val.get())->getValueString(widths);
				std::cout << "\t@" << ident << "_" << st_cur->tag << " = alloc " << array_type << std::endl;

				// 利用getelemptr得到指针, 然后store初始化数据
				// dim个getelemptr, 本质是dim层循环
				for (int i = 0; i < init_values.size(); i++)
				{
					std::vector<int32_t> offsets(dim, 0);
					int tmp = i;
					for (int j = 0; j < dim; j++)
					{
						offsets[j] = tmp / suf_mul[j];
						tmp = tmp % suf_mul[j];

						if (j == 0)
						{
							std::cout << "\t%" << reg_cnt << " = getelemptr @" << ident << "_" << st_cur->tag << ", " << offsets[j] << std::endl;
							reg_cnt++;
						}
						else
						{
							std::cout << "\t%" << reg_cnt << " = getelemptr %" << reg_cnt - 1 << ", " << offsets[j] << std::endl;
							reg_cnt++;
						}
					}

					std::cout << "\tstore " << init_values[i] << ", %" << reg_cnt - 1 << std::endl;
				}
			}
		}

		return std::string();
	}
};

// ...
