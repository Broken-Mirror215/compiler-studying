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
%token CONST
%token IF ELSE
%token WHILE BREAK CONTINUE
%token VOID 


// 非终结符的类型定义
//ds v4 flash 3.1
%type <ast_val> FuncDef FuncType Block Stmt  Number 
%type <ast_val>Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <int_val> UnaryOp MulOp AddOp
%type <ast_val> BlockStmtList BlockStmt
%type <ast_val>ConstDecl ConstDefList ConstDef
%type <ast_val>ConstInitVal ConstExp LVal
%type <ast_val>VarDecl VarDefList VarDef InitVal
%type <ast_val>OtherStmt MatchedStmt UnmatchedStmt 
%type <ast_val> Program CompUnit FuncFParams FuncRParams
%type <str_val>FuncFParam
%% 

//------------------------------------------------------------------------上面是配置和声明
// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值

//lab 8.1
Program
: CompUnit {
  ast=unique_ptr<BaseAst> ($1);
}
;
//lab 8.1 这部分我的理解是递归，找到所有的函数定义
CompUnit  //这是整个编译单元，目前只有一个函数
  : FuncDef {
    auto node = new CompUnitAst();
    node->funcs.push_back(unique_ptr<BaseAst>($1));
    $$ = node;
}
| CompUnit FuncDef {
  auto node= static_cast<CompUnitAst*>($1);
  node->funcs.push_back(unique_ptr<BaseAst>($2));
  $$ = node;
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

//lab 8.1
FuncDef 
: FuncType IDENT '(' ')' Block {
  auto ast=new FuncDefAst();
  ast->func_type=unique_ptr<BaseAst>($1);
  ast->ident=*unique_ptr<string>($2);
  ast->block=unique_ptr<BaseAst>($5);
  //下面这个呢？
  $$ =ast; 
} 
| FuncType IDENT  '(' FuncFParams')' Block {
  auto ast = new FuncDefAst();
  ast->func_type = unique_ptr<BaseAst>($1);
  ast->ident = *unique_ptr<string>($2);
  //形参列表就是个中转站，名字取走后就地销毁
  auto params = unique_ptr<FuncFParamsAst>(static_cast<FuncFParamsAst*>($4));
  ast->params = move (params->params);//搬入形参名字？
  ast->block = unique_ptr<BaseAst>($6);
  $$ = ast;
}
;


//lab8.1
FuncType
: INT {
  $$ =new FuncTypeAst();
}
| VOID {
  auto node = new FuncTypeAst();
  node->is_void=true;
  $$ = node;
}
;

//lab 8.1 形参只要名字，所以函数只要返回一个字符串指针?
FuncFParam
:INT IDENT {
  $$ = $2;
}
;
//lab 8.1
FuncFParams
:FuncFParam {
  auto node = new FuncFParamsAst();
  node->params.push_back(*unique_ptr<string>($1));
  $$ = node;
}
| FuncFParams ',' FuncFParam {
  auto node = static_cast<FuncFParamsAst*> ($1);
  node->params.push_back(*unique_ptr<string>($3));
  $$ = node; 
}
;


Block //这是一个基本块长什么样子
  : '{' BlockStmtList '}' {
      $$=$2;
  }
  ;
//这是基本块内容，可以是一个基本块，也可以用list链接起来
BlockStmtList
:%empty{
  $$=new BlockAst();
}
| BlockStmtList BlockStmt {
  auto block =static_cast<BlockAst*>($1);
  block->stmts.push_back(unique_ptr<BaseAst>($2));
  $$=block;
}

//这就是一个基本块长什么样子了.目前我写了常量 变量 和 在stmt里面找到的return 
BlockStmt
:ConstDecl {
  $$=$1;
}
|VarDecl {
  $$=$1;
}
|Stmt {
  $$=$1;
}
;


Stmt 
:MatchedStmt{
  $$=$1;
}
|UnmatchedStmt
{
  $$=$1;
}
; 

MatchedStmt  //匹配的情况包含，完整的语句块
:OtherStmt {
  $$=$1;
}
| IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
  auto node =new IfStmtAst();
  node->cond = unique_ptr<BaseAst>($3);
  node->then_ast=unique_ptr<BaseAst>($5);
  node->else_ast=unique_ptr<BaseAst>($7);
  $$=node;
}
|WHILE '(' Exp ')' MatchedStmt {
  auto node =new WhileStmtAst();
  node->cond = unique_ptr<BaseAst>($3);
  node->body=unique_ptr<BaseAst>($5);
  $$=node;
}
;

UnmatchedStmt  //不匹配的情况就是只有一个 if  他是怎么变成block的呢？他其实是先走的stmt,然后变成了blockstmt再利用外面的{}变成了block，block自己的规则就是变成ohterblock然后再变成matchedstmt
:IF '(' Exp ')' Stmt {
  auto node =new IfStmtAst();
  node->cond =unique_ptr<BaseAst>($3);
  node->then_ast = unique_ptr<BaseAst> ($5);
  $$=node;
}               //或者 else 里面还有else
|IF '(' Exp ')' MatchedStmt ELSE UnmatchedStmt {
  auto node =new IfStmtAst();
  node->cond = unique_ptr<BaseAst>($3);
  node->then_ast=unique_ptr<BaseAst>($5);
  node->else_ast=unique_ptr<BaseAst>($7);
  $$=node;
}
|WHILE '(' Exp ')' UnmatchedStmt {
  auto node =new WhileStmtAst();
  node->cond = unique_ptr<BaseAst>($3);
  node->body=unique_ptr<BaseAst>($5);
  $$=node;
}
;


//这里就代指语句
OtherStmt
  //在3.1的时候，如果Return后面返回的是一个表达式，那就会变成一棵树。所以需要指针
: RETURN Exp ';' {
  auto node=new StmtAst();
  node->expr=unique_ptr<BaseAst>($2);
  $$=node;
}
| RETURN ';' {
  auto node = new StmtAst();
  $$ = node;
}
| LVal '=' Exp ';' {
  auto node = new AssignStmtAst();
  unique_ptr<LValAst> lval (
      static_cast<LValAst*> ($1)
  );
  node->ident = lval->ident;
  node->expr = unique_ptr<BaseAst>($3);
  $$ = node;
}
| Block{
  $$=$1;
}
| Exp ';' {
  auto node=new ExprStmtAst();
  node->expr = unique_ptr<BaseAst>($1);
  $$=node;
}
| ';' {
  $$ = new ExprStmtAst();
}
| BREAK ';' {
  $$=new BreakStmtAst();
}
| CONTINUE ';' {
  $$=new ContinueStmtAst();
}
;

//这里就代指int了
Number
: INT_CONST {
  auto node=new NumberAst();
  node->number=$1;
  $$=node; //BaseAst& p = node;
}
;

//这里是一个基本表达式， 长得就像a + 2 * 3;
Exp
: LOrExp{
  $$=$1;
}
;

//这里就是一个基本表达式长相可以是 (123) 23 a 这样子
PrimaryExp
:'(' Exp ')' {
  $$=$2;
}
| Number{
  $$=$1;
}
| LVal {
  $$=$1;
}
;

//这里是一元表达式，可以是+a -a !a也可以是一些基本表达式。
UnaryExp //一元表达式喵 +a. -a,!a这些都是喵！
:PrimaryExp {
  $$=$1;
}
| UnaryOp UnaryExp{
  auto node=new UnaryExpAst();
  node->op=$1;
  node->operand=unique_ptr<BaseAst>($2); 
  $$=node;
}
/* lab 8.1 */
| IDENT '(' ')' {
  auto node = new FuncCallAst();
  node->ident =*unique_ptr<string>($1);
  $$ = node;
}
| IDENT '(' FuncRParams ')' {
  auto node = new FuncCallAst();
  node->ident = *unique_ptr<string>($1);
  auto args = unique_ptr<FuncRParamsAst>(static_cast<FuncRParamsAst*> ($3));
  node->args = move(args->args);
  $$ = node;
}
;

//lab 8.1 这个部分是实参？
FuncRParams
:Exp {
  auto node = new FuncRParamsAst();
  node->args.push_back(unique_ptr<BaseAst>($1));
  $$ = node;
}
| FuncRParams ',' Exp {
  auto node = static_cast<FuncRParamsAst*>($1);
  node->args.push_back(unique_ptr<BaseAst>($3));
  $$ = node;
}
;


//这里就做了运算符的替换，不会生成ir
UnaryOp
:'+' { $$='+';}
|'-' {$$ ='-';}
|'!' {$$= '!';}
;

//这里是加/减法表达式 a + b 和a - b 因为addop在下面变形了-
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
//这里是乘法表达式 就是 a * b什么的 但是可以当做一元表达式
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

RelExp //这一块是比大小的
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


EqExp //这一块是比等于还是不等于的 a==b a!=b
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

//这是逻辑与这一层
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
//这是逻辑或这一层
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

ConstDecl //这里是常量声明 const int a=3 b=3 c=3 因为后面是一个list
:CONST INT ConstDefList ';' {
  $$=$3;
}
;

VarDecl //这里是变量的声明 int a=3 b=4 因为最基本的vardef兼容了两个版本，所以都有可能
: INT VarDefList ';' {
  $$=$2;
}

ConstDefList //这里是常量定义列表 const int a=3 b=3 c=3
:ConstDef {
  auto node =new ConstDeclAst();
  node->defs.push_back(unique_ptr<BaseAst>($1));
  $$=node;
}
| ConstDefList ',' ConstDef {
  auto node=static_cast<ConstDeclAst*>($1);
  node->defs.push_back(unique_ptr<BaseAst>($3));
  $$=node;
}
;

VarDefList //这里就是变量列表 int a=3,b=4
:VarDef {
  auto node =new VarDeclAst();
  node->defs.push_back(unique_ptr<BaseAst>($1));
  $$=node;
}
| VarDefList ',' VarDef {
  auto node = static_cast<VarDeclAst*>($1);
  node->defs.push_back(unique_ptr<BaseAst>($3));
  $$ = node;
}
;

ConstDef //这里写的是常量赋值 const int a =3 中的 a =3 
:IDENT '=' ConstInitVal {
  auto node = new ConstDefAst();
  unique_ptr<string> name($1);
  node->ident =*name;
  node->init=unique_ptr<BaseAst>($3);

  $$=node;
}

VarDef  //这里就是一个变量 a
:IDENT {
  auto node =new VarDefAst();
  unique_ptr<string> name ($1);
  node->ident=*name;
  $$=node;
}
| IDENT '=' InitVal { //这里是变量赋值 int a = 3里面的 a = 3
  auto node =new VarDefAst();
  unique_ptr<string> name ($1);
  node->ident=*name;
  node->init=unique_ptr<BaseAst>($3);
  $$=node;
}
;

ConstInitVal //这是常量的初始值 const int a = 1 + 2里面的 1 + 2
:ConstExp {
  $$=$1;//
}
;

InitVal
:Exp {
  $$=$1;
}
;

ConstExp //这是常量的表达式 用 1 + 2,后续在编译期计算结果
:Exp{
  $$=$1;
}
;

LVal //????这是最没搞懂的，是名字的使用 a = 3 就是左边的 a 或者就是 return a 里面的 a?
:IDENT {  
  auto node =new LValAst();
  unique_ptr<string> name($1);
  node->ident = *name;
  $$ = node;
}




%% //-----------------------------------------------------------------书写我的语法规则和对应动作

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAst> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
