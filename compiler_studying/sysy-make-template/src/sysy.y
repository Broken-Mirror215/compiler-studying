%code requires {
  #include <memory>
  #include <string>
  #include "../AST/BaseAst.h" //必须要放在这里？
}

%{

#include "../AST/BaseAst.h"
#include <iostream>
#include <memory>
#include <string>
// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAst> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAst> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAst * ast_val;

}

// lexer 返回的所有 token 种类的声明,终结符
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST
%token LE GE EQ NE LAND LOR

// 非终结符的类型定义
//ds v4 flash 3.1
%type <ast_val> FuncDef FuncType Block Stmt  Number 
%type <ast_val>Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <int_val> UnaryOp MulOp AddOp


%% 

//------------------------------------------------------------------------上面是配置和声明
// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit=make_unique<CompUnitAst>();
    comp_unit->FuncDef=unique_ptr<BaseAst>($1);
    ast=move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast=new FuncDefAst();
    ast->func_type=unique_ptr<BaseAst>($1);
    ast->ident=*unique_ptr<string>($2);
    ast->block=unique_ptr<BaseAst>($5);
    //下面这个呢？
    $$ =ast; //派生类赋值给了基类
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    $$ =new FuncTypeAst();
  }
  ;

Block
  : '{' Stmt '}' {
    auto node =new BlockAst();
    node->stmt=unique_ptr<BaseAst>($2);
    $$=node;
  }
  ;

Stmt
  //在3.1的时候，如果Return后面返回的是一个表达式，那就会变成一棵树。所以需要指针
  : RETURN Exp ';' {
    auto node=new StmtAst();
    node->expr=unique_ptr<BaseAst>($2);
    $$=node;
  }
  ;


Number
  : INT_CONST {
    auto node=new NumberAst();
    node->number=$1;
    $$=node;
  }
  ;

//ds v4 flash 3.1
Exp
: LOrExp{
  $$=$1;//说是接优先级最低的...也是因为递归先处理。
}
;

//ds v4 flash 3.1
PrimaryExp
:'(' Exp ')' {
  $$=$2;
}
| Number{
  $$=$1;
}
;

//ds v4 flash 3.1
UnaryExp
:PrimaryExp {
  $$=$1;
}
| UnaryOp UnaryExp{
  auto node=new UnaryExpAst();
  node->op=$1;
  node->operand=unique_ptr<BaseAst>($2); 
  $$=node;
}
;


//ds v4 flash
UnaryOp
:'+' { $$='+';}
|'-' {$$ ='-';}
|'!' {$$= '!';}
;

AddExp
:MulExp{
  $$=$1;
}
| AddExp AddOp MulExp {
  auto node = new BinaryExpAst();
  node->op = string(1,static_cast<char>($2));
  node->lhs=unique_ptr<BaseAst>($1);
  node->rhs=unique_ptr<BaseAst>($3);
  $$=node;
}
//这里有一个小坑,只能左递归，因为在c语言中"- "是左结合。

MulExp
:UnaryExp{
  $$=$1;
}
| MulExp MulOp UnaryExp {
  auto node=new BinaryExpAst();
  node->op=string(1,static_cast<char>($2));
  node->lhs=unique_ptr<BaseAst>($1);
  node->rhs=unique_ptr<BaseAst>($3);
  $$=node;
}
;

AddOp
:'+' {$$='+';}
|'-' {$$='-';}
;
MulOp
:'*' {$$='*';}
|'/' {$$='/';}
|'%' { $$='%';}
;

RelExp
: AddExp {
  $$ = $1;
}
| RelExp '<' AddExp {
  auto node = new BinaryExpAst();
  node->op = "<";
  node->lhs = unique_ptr<BaseAst>($1);
  node->rhs = unique_ptr<BaseAst>($3);
  $$ = node;
}
| RelExp '>' AddExp {
  auto node = new BinaryExpAst();
  node->op = ">";
  node->lhs = unique_ptr<BaseAst>($1);
  node->rhs = unique_ptr<BaseAst>($3);
  $$ = node;
}
| RelExp LE AddExp {
  auto node = new BinaryExpAst();
  node->op = "<=";
  node->lhs = unique_ptr<BaseAst>($1);
  node->rhs = unique_ptr<BaseAst>($3);
  $$ = node;
}
| RelExp GE AddExp {
  auto node = new BinaryExpAst();
  node->op = ">=";
  node->lhs = unique_ptr<BaseAst>($1);
  node->rhs = unique_ptr<BaseAst>($3);
  $$ = node;
}
;


EqExp
: RelExp {
  $$ = $1;
}
| EqExp EQ RelExp {
  auto node = new BinaryExpAst();
  node->op = "==";
  node->lhs = unique_ptr<BaseAst>($1);
  node->rhs = unique_ptr<BaseAst>($3);
  $$ = node;
}
| EqExp NE RelExp {
  auto node = new BinaryExpAst();
  node->op = "!=";
  node->lhs = unique_ptr<BaseAst>($1);
  node->rhs = unique_ptr<BaseAst>($3);
  $$ = node;
}
;

LAndExp
:EqExp {
  $$=$1;
}
|LAndExp LAND EqExp {
  auto node =new BinaryExpAst();
  node->op="&&";
  node->lhs= unique_ptr<BaseAst>($1);
  node->rhs= unique_ptr<BaseAst>($3);
  $$ = node;
}
;

LOrExp 
: LAndExp {
  $$=$1;
}
| LOrExp LOR LAndExp {
  auto node =new BinaryExpAst();
  node->op="||";
  node->lhs= unique_ptr<BaseAst>($1);
  node->rhs= unique_ptr<BaseAst>($3);
  $$ = node;
}
;




%% //-----------------------------------------------------------------书写我的语法规则和对应动作

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAst> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
