%code requires {
  #include <memory>
  #include <string>
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "AST.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}


%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// union中不能有默认构造函数??
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>>* vec_val;
}

// lexer 返回的所有 token 种类的声明
%token INT RETURN LE GE EQ NEQ LAND LOR CONST
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt
%type <ast_val> Exp PrimaryExp UnaryExp UnaryOp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl BType ConstDef ConstInitVal BlockItem LVal ConstExp VarDecl VarDef InitVal
%type <vec_val> BlockItemList ConstDefList VarDefList
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


// 右值引用/move语义在汇编层面如何实现的??
// unique_ptr只能move
// 注意这里vector怎么不能向IDENT那样解引用赋值??
Block
: '{' BlockItemList '}' 
{
  auto ast = new BlockAST();
  auto tmp = $2;
  ast->block_items = vector<unique_ptr<BaseAST>>();

  // unique_ptr 必须是引用
  for(auto& i : *tmp)
  {
    ast->block_items.push_back(move(i));
  }
  delete tmp;
  //ast->block_items = *unique_ptr<vector<unique_ptr<BaseAST>>>($2);
  $$ = ast;
};

BlockItemList
:
{
  auto tmp = new vector<unique_ptr<BaseAST>>();
  $$ = tmp;
}
| BlockItemList BlockItem // 使用左递归更好???
{
  auto tmp = $1;// move
  tmp->push_back(unique_ptr<BaseAST>($2));
  $$ = tmp;
};

BlockItem
: Decl
{
  auto ast = new BlockItemAST();
  ast->type = BlockItemAST::DECL;
  ast->decl = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| Stmt
{
  auto ast = new BlockItemAST();
  ast->type = BlockItemAST::STMT;
  ast->stmt = unique_ptr<BaseAST>($1);
  $$ = ast;
};

Stmt
: RETURN Exp ';' 
{
  auto ast = new StmtAST();
  ast->type = StmtAST::ONE_RETURN;
  ast->exp = unique_ptr<BaseAST>($2);
  $$ = ast;
}
| LVal '=' Exp ';'
{
  auto ast = new StmtAST();
  ast->type = StmtAST::LVAL;
  ast->lval = unique_ptr<BaseAST>($1);
  ast->exp = unique_ptr<BaseAST>($3);
  $$ = ast;
}
| Exp ';'
{
  auto ast = new StmtAST();
  ast->type = StmtAST::ONE_EXP;
  ast->exp = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| ';'
{
  auto ast = new StmtAST();
  ast->type = StmtAST::ZERO_EXP;
  $$ = ast;
}
| Block
{
  auto ast = new StmtAST();
  ast->type = StmtAST::BLOCK;
  ast->block = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| RETURN ';'
{
  auto ast = new StmtAST();
  ast->type = StmtAST::ZERO_RETURN;
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
}
| LVal
{
  auto ast = new PrimaryExpAST();
  ast->lval = unique_ptr<BaseAST>($1);
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


// ================================= lv4 ===========================
Decl
: ConstDecl
{
  auto ast = new DeclAST();
  ast->type = DeclAST::CONST;
  ast->const_decl = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| VarDecl
{
  auto ast = new DeclAST();
  ast->type = DeclAST::VAR;
  ast->var_decl = unique_ptr<BaseAST>($1);
  $$ = ast;
};

ConstDecl
: CONST BType ConstDefList ';'
{
  auto ast = new ConstDeclAST();
  ast->btype = unique_ptr<BaseAST>($2);
  // vector怎么初始化?????
  auto tmp = $3;
  for(auto& i : *tmp)
  {
    ast->const_defs.push_back(move(i));
  }
  //ast->const_defs = *unique_ptr<vector<unique_ptr<BaseAST>>>($3);
  delete tmp;
  $$ = ast;
};

ConstDefList
: ConstDef
{
  auto tmp = new vector<unique_ptr<BaseAST>>();
  tmp->push_back(unique_ptr<BaseAST>($1));
  $$ = tmp;
}
| ConstDefList ',' ConstDef
{
  auto tmp = $1;
  tmp->push_back(unique_ptr<BaseAST>($3));
  $$ = tmp;
}

BType
: INT
{
  auto ast = new BTypeAST();
  $$ = ast;
};

ConstDef
: IDENT '=' ConstInitVal
{
  auto ast = new ConstDefAST();
  // string的处理
  ast->ident = *unique_ptr<string>($1);
  ast->const_init_val = unique_ptr<BaseAST>($3);
  $$ = ast;
};

ConstInitVal
: ConstExp
{
  auto ast = new ConstInitValAST();
  ast->const_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
};

LVal
: IDENT
{
  auto ast = new LValAST();
  // string
  ast->ident = *unique_ptr<string>($1);
  $$ = ast;
};

ConstExp
: Exp
{
  auto ast = new ConstExpAST();
  ast->exp = unique_ptr<BaseAST>($1);
  $$ = ast;
};

VarDecl
: BType VarDefList ';'
{
  auto ast = new VarDeclAST();
  ast->btype = unique_ptr<BaseAST>($1);
  auto tmp = $2;
  for(auto& i : *tmp)
  {
    ast->var_defs.push_back(move(i));
  }
  delete tmp;
  $$ = ast;
};

VarDefList
: VarDef
{
  auto tmp = new vector<unique_ptr<BaseAST>>();
  tmp->push_back(unique_ptr<BaseAST>($1));
  $$ = tmp;
}
| VarDefList ',' VarDef
{
  auto tmp = $1;
  tmp->push_back(unique_ptr<BaseAST>($3));
  $$ = tmp;
};

VarDef
: IDENT
{
  auto ast = new VarDefAST();
  ast->type = VarDefAST::IDENT;
  ast->ident = *unique_ptr<string>($1);
  $$ = ast;
}
| IDENT '=' InitVal
{
  auto ast = new VarDefAST();
  ast->type = VarDefAST::INIT;
  ast->ident = *unique_ptr<string>($1);
  ast->init_val = unique_ptr<BaseAST>($3);
  $$ = ast;
};

InitVal
: Exp
{
  auto ast = new InitValAST();
  ast->exp = unique_ptr<BaseAST>($1);
  $$ = ast;
};



%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
