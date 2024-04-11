%code requires {
  #include <memory>
  #include <string>
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}


%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
%token INT RETURN LE GE EQ NEQ LAND LOR
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt
%type <ast_val> Exp PrimaryExp UnaryExp UnaryOp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <int_val> Number

%%


CompUnit
: FuncDef 
{
  auto comp_unit = make_unique<CompUnitAST>();
  comp_unit->func_def = unique_ptr<BaseAST>($1);
  ast = move(comp_unit);
};


FuncDef
: FuncType IDENT '(' ')' Block
{
  auto ast = new FuncDefAST();
  ast->func_type = unique_ptr<BaseAST>($1);
  ast->ident = *unique_ptr<string>($2);
  ast->block = unique_ptr<BaseAST>($5);
  $$ = ast;
};


FuncType
: INT 
{
  auto ast = new FuncTypeAST();
  $$ = ast;
};

Block
: '{' Stmt '}' 
{
  auto ast = new BlockAST();
  ast->stmt = unique_ptr<BaseAST>($2);
  $$ = ast;
};

Stmt
: RETURN Exp ';' 
{
  auto ast = new StmtAST();
  ast->exp = unique_ptr<BaseAST>($2);
  $$ = ast;
};

Number
: INT_CONST 
{
  $$ = $1;
};





// ================================= lv3 ===========================
Exp 
: LOrExp
{
  auto ast = new ExpAST();
  ast->lor_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
};

PrimaryExp
: '(' Exp ')'
{
  auto ast = new PrimaryExpAST();
  ast->exp = unique_ptr<BaseAST>($2);
  $$ = ast;
}
| Number
{
  auto ast = new PrimaryExpAST();
  ast->number = $1;
  $$ = ast;
};

UnaryExp
: PrimaryExp
{
  auto ast = new UnaryExpAST();
  ast->primary_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| UnaryOp UnaryExp
{
  auto ast = new UnaryExpAST();
  ast->unary_op = unique_ptr<BaseAST>($1);
  ast->unary_exp = unique_ptr<BaseAST>($2);
  $$ = ast;
};

UnaryOp
: '+'
{
  auto ast = new UnaryOpAST();
  ast->op = '+';
  $$ = ast;
}
| '-'
{
  auto ast = new UnaryOpAST();
  ast->op = '-';
  $$ = ast;
}
| '!'
{
  auto ast = new UnaryOpAST();
  ast->op = '!';
  $$ = ast;
};

MulExp
: UnaryExp
{
  auto ast = new MulExpAST();
  ast->unary_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| MulExp '*' UnaryExp
{
  auto ast = new MulExpAST();
  ast->mul_exp = unique_ptr<BaseAST>($1);
  ast->op = '*';
  ast->unary_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| MulExp '/' UnaryExp
{
  auto ast = new MulExpAST();
  ast->mul_exp = unique_ptr<BaseAST>($1);
  ast->op = '/';
  ast->unary_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| MulExp '%' UnaryExp
{
  auto ast = new MulExpAST();
  ast->mul_exp = unique_ptr<BaseAST>($1);
  ast->op = '%';
  ast->unary_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
};

AddExp
: MulExp
{
  auto ast = new AddExpAST();
  ast->mul_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| AddExp '+' MulExp
{
  auto ast = new AddExpAST();
  ast->add_exp = unique_ptr<BaseAST>($1);
  ast->op = '+';
  ast->mul_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| AddExp '-' MulExp
{
  auto ast = new AddExpAST();
  ast->add_exp = unique_ptr<BaseAST>($1);
  ast->op = '-';
  ast->mul_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
};

RelExp
: AddExp
{
  auto ast = new RelExpAST();
  ast->type = RelExpAST::ADD;
  ast->add_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| RelExp '<' AddExp
{
  auto ast = new RelExpAST();
  ast->type = RelExpAST::L;
  ast->rel_exp = unique_ptr<BaseAST>($1);
  ast->add_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| RelExp '>' AddExp
{
  auto ast = new RelExpAST();
  ast->type = RelExpAST::G;
  ast->rel_exp = unique_ptr<BaseAST>($1);
  ast->add_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| RelExp LE AddExp
{
  auto ast = new RelExpAST();
  ast->type = RelExpAST::LE;
  ast->rel_exp = unique_ptr<BaseAST>($1);
  ast->add_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| RelExp GE AddExp
{
  auto ast = new RelExpAST();
  ast->type = RelExpAST::GE;
  ast->rel_exp = unique_ptr<BaseAST>($1);
  ast->add_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
};

EqExp
: RelExp
{
  auto ast = new EqExpAST();
  ast->type = EqExpAST::REL;
  ast->rel_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| EqExp EQ RelExp
{
  auto ast = new EqExpAST();
  ast->type = EqExpAST::EQ;
  ast->eq_exp = unique_ptr<BaseAST>($1);
  ast->rel_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| EqExp NEQ RelExp
{
  auto ast = new EqExpAST();
  ast->type = EqExpAST::NEQ;
  ast->eq_exp = unique_ptr<BaseAST>($1);
  ast->rel_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
};

LAndExp
: EqExp
{
  auto ast = new LAndExpAST();
  ast->type = LAndExpAST::EQ;
  ast->eq_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| LAndExp LAND EqExp
{
  auto ast = new LAndExpAST();
  ast->type = LAndExpAST::LAND;
  ast->land_exp = unique_ptr<BaseAST>($1);
  ast->eq_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
};

LOrExp
: LAndExp
{
  auto ast = new LOrExpAST();
  ast->type = LOrExpAST::LAND;
  ast->land_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| LOrExp LOR LAndExp
{
  auto ast = new LOrExpAST();
  ast->type = LOrExpAST::LOR;
  ast->lor_exp = unique_ptr<BaseAST>($1);
  ast->land_exp = unique_ptr<BaseAST>($3);
  $$ = ast;
};



%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
