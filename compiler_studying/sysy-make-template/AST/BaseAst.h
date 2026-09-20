#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
using namespace std;
//ds v4 flash 3.1
//这个函数用来干什么的？？
//是一次返回"%0" "%1" 每次load或运算生成一个结果名字给koopa ir
inline string NewTemp(){
    static int cnt=0;
    return "%" + to_string(cnt++);
}

inline string NewVarAddr (const string & ident){
    static int cnt=0;
    return "%var_" + ident + "_" + to_string(cnt++);
}

//基本块命名函数
inline string NewLabel (const string & kind) {
    static int cnt=0;
    return "%bb_" + kind + "_" + to_string(cnt++); 
}

enum class SymbolKind {
    Constant,
    Variable
};


class SymbolInfo {
public:
    SymbolKind kind;
    int const_value = 0;//常量使用这个字段
    string addr; //变量使用这个字段 保存koopa存储位置的数字 例如 "@x"
};

struct LoopInfo {
    string cond_label;
    string end_label;
};


//使用数组手写模拟栈！
inline vector<LoopInfo> & LoopStack() {
    static vector<LoopInfo> loops;
    return loops;
}







inline vector<unordered_map<string,SymbolInfo>> & ScopeStack() {
    static vector<unordered_map<string,SymbolInfo>> scopes;
    return scopes;
}

//进入一个作用域就弄一个空的元素
inline void EnterScope () {
    ScopeStack().emplace_back();
}

//离开一个作用域的时候，就删除最内层的表
inline void ExitScope() {
    auto & scopes = ScopeStack();

    if (scopes.empty()){
        throw logic_error ("没有可以退出的作用域");
    }

    scopes.pop_back();
}


inline unordered_map<string,SymbolInfo> & SymbolTable(){
    auto&scopes = ScopeStack();

    if (scopes.empty()){
        throw logic_error ("当前没有作用域");
    }

    return scopes.back();
}


//查名字要求返回的完整的符号信息,由于后面的作用域，我们建造了多张符号表，然后根据退出的原则，我们查表必须从最里面的表开始查
inline const SymbolInfo& LookupSymbol(const string & name){
    const auto & scopes = ScopeStack();
    
    for (auto scope=scopes.rbegin();scope!=scopes.rend();scope++){
        auto it=scope->find(name);
        if (it !=scope->end()){ //这里为什么是->，是因为数组问题吗？
            return it->second;
        }
    }
    throw runtime_error ("使用了未定义的标识符: "+ name);
}

//查名字，并且要求他必须是常量，然后返回一个整数值，由于后面的作用域，我们建造了多张符号表，然后根据退出的原则，我们查表必须从最里面的表开始查
inline int LookupConst(const string & name){
    const auto & symbol= LookupSymbol(name);
    if (symbol.kind!=SymbolKind::Constant){
        throw runtime_error("常量表达式中使用了变量: "+name);
    }

    return symbol.const_value;
}

//ds v4 flash 3.1
inline string EmitBinary(const string &op,const string & lhs,const string & rhs){
    string tmp=NewTemp();
    cout << " " << tmp << " = " << op << " " << lhs << ", "<< rhs << "\n";
    return tmp;
}

struct BaseAst{
public:
    virtual ~BaseAst()=default; //析构函数怎么设置成虚函数了？
    virtual void Dump() const = 0;

    //ds v4 flash 3.1
    mutable string result;

    virtual int Calc() const { //因为加减和乘除都会用，在编译期求值
        throw logic_error("这个Ast节点不能作为常量表达式求值!!");
    }

    //这个函数做什么用来着？
    virtual bool IsTerminated () const {
        return false;
    }
};

//这是生成ir的入口。
class CompUnitAst : public BaseAst{
public:
    //一串函数
    vector<unique_ptr<BaseAst>> funcs;
    // void Dump() const override {
    //     cout << "CompUnitAST { ";
    //     FuncDef->Dump();
    //     cout << " }";
    // }

    void Dump() const override{
        //每次处理就清空一下符号表。
        ScopeStack().clear();
        LoopStack().clear();
        for (auto & func : funcs)
            func->Dump();
    }
};

class FuncDefAst : public BaseAst{
public:
    unique_ptr<BaseAst> func_type;
    string ident;
    unique_ptr<BaseAst> block;
    //存形参名字
    vector<string> params;
    // void Dump() const override {
    //     cout << "FuncDefAST { ";
    //     func_type->Dump();
    //     cout << ", " << ident << ", ";
    //     block->Dump();
    //     cout << " }";
    // }

    //lab 8.1
    void Dump() const override{
        func_type->Dump(); //这一段为什么会有？
        cout << "fun @" << ident << "(";
        for (size_t i=0;i<params.size();i++) {
            if (i>0) cout << ", ";
            cout << "@" << params[i] << ": i32";
        }
        cout << ")" << func_type->result << " {\n";
        cout<< "%entry:\n";
        block->Dump();
        cout << "}\n";
    }
};

 class FuncTypeAst : public BaseAst {
   public:
    // void Dump() const override {
    //     cout << "FuncTypeAST { int }";
    // }
    bool is_void =false;
    void Dump() const override {
        result = is_void ? "" : ": i32";
    }
};


class StmtAst : public BaseAst {
   public:

   unique_ptr<BaseAst> expr;
    //int number = 0;
    // void Dump() const override {
    //     cout << "StmtAST { " << number << " }";
    // }

    // void Dump() const override{
    //     cout<<" ret "<< number <<"\n";
    // }
    

    //lab 8.1
    void Dump() const override{
        if (expr) {
            expr->Dump();
            cout << " ret " << expr->result << "\n";
        } else {
            cout << " ret\n";
        }
    }
    
    bool IsTerminated () const override{
        return true;
    }
};

class ExprStmtAst : public BaseAst {
public:
    unique_ptr<BaseAst> expr;
    void Dump () const override {
        if (expr) {
            expr->Dump();
        }
    }
};

//lab 8.1 形参列表的载体，自己不生成ir 由FuncDefAst取走里面生成的数字
class FuncFParamsAst : public BaseAst {
public:
    vector<string> params;
    void Dump() const override {}
};
//lab 8.1 实参列表的载体 由函数调用那条规则取走？
class FuncRParamsAst : public BaseAst {
public:
    vector<unique_ptr<BaseAst>> args;
    void Dump() const override {}
};

class FuncCallAst : public BaseAst {
public:
    string ident;
    vector<unique_ptr<BaseAst>> args;
    void Dump () const override {}
};

class IfStmtAst : public BaseAst {
public:
    unique_ptr<BaseAst> cond;
    unique_ptr<BaseAst> then_ast;
    unique_ptr<BaseAst> else_ast;

    mutable bool terminater =false;
    void Dump() const override {
        terminater= false;

        string then_label =NewLabel("then");
        string end_label =NewLabel("end");
        string else_label;

        if (else_ast) {
            else_label=NewLabel("else");
        }
        

        //1.计算条件，生成条件跳转
        cond->Dump();
        cout << "br " << cond->result << ", " << then_label << ", " << (else_ast? else_label : end_label) << "\n"; 
    
        //2.生成then 分支
        cout << then_label << ":\n";
        then_ast->Dump();

        bool then_terminated = then_ast->IsTerminated();
        if (!then_terminated) {
            cout << " jump " << end_label << "\n";
        }

        //3.如果存在else 分支就要生成一个else 分支
        bool else_terminated =false;
        if (else_ast) {
            cout << else_label << ":\n";
            else_ast->Dump();

            else_terminated =else_ast->IsTerminated();

            if (!else_terminated){
                cout << " jump " <<end_label << "\n";
            }
        }
        //4.两个分支都终止，整个if 才终止
        terminater =else_ast!=nullptr && then_terminated &&else_terminated;

        //5.还有路径就继续执行
        if (!terminater) {
            cout << end_label << ":\n";
        }
    }

    bool IsTerminated () const override {
        return terminater;
    }
};



class BlockAst : public BaseAst {
   public:
    //unique_ptr<BaseAst> stmt;
    // void Dump() const override {
    //     cout << "BlockAST { ";
    //     stmt->Dump();
    //     cout << " }";
    // }

    mutable bool terminater =false;
    vector<unique_ptr<BaseAst>> stmts;

    void Dump() const override {
        EnterScope();
        terminater=false;
        for (auto & stmt:stmts){
            stmt->Dump();

             if (stmt->IsTerminated()){
                terminater=true;
                break;
             }
        }       
        ExitScope();
    }

    bool IsTerminated () const override {
        return terminater;
    }
};


class NumberAst : public BaseAst{
public:
    int number=0;
    //numberAst相当于子树的节点了，所以这里不需要创建一个节点
    //ds 4 flash 3.1
    void Dump() const override{
        result=to_string(number);
    }

    //等到数字节点的指针用了这个函数才会返回...
    int Calc() const {
        return number;
    }
};

//二元运算模拟一元运算
class UnaryExpAst : public BaseAst{
public:
    char op;
    unique_ptr<BaseAst> operand;

    //ds 4 flash 3.1
    void Dump() const override{
        operand->Dump();//这不一定会调用基类的函数，会根据实际的对象类型。
        string value =operand->result;
        if (op=='+'){
            result = value;//+x不生成ir
        }
        else if (op=='-'){
            result=EmitBinary("sub","0",value); //-x 就是0-x
        }
        else {
            result =EmitBinary("eq",value,"0");
        }
    }

    int Calc() const override {
        int value=operand->Calc();
        if (op=='+') return value;
        if (op=='-') return -value;
        if (op=='!') return !value;

        throw logic_error("未知的一元运算符！");
    }
};


//ds 4 flash 3.2
//二元表达式 左右都要弄一颗子树
//这里不用优先级判断，因为优先级会在AddExp和MulExp分层？
class BinaryExpAst : public BaseAst {
public:
    string op;
    unique_ptr<BaseAst> lhs;
    unique_ptr<BaseAst> rhs;

    void Dump() const override {

        lhs->Dump();

        
        if (op=="&&" || op == "||") {
            bool is_and = (op=="&&");
        

        string addr =NewVarAddr("logic");
        string rhs_label = NewLabel("logic_rhs");
        string end_label = NewLabel("logic_end");

      
            cout << " " << addr << " = alloc i32\n";
            //&&默认用0结果 || 默认用1结果
            cout << " store " << (is_and ? 0 : 1) << ", " <<addr << "\n";


            //br的两个目标依次对应，条件非0,条件是0 
            //到了risc-v的时候就会变成bnez什么的了。
            cout << " br " << lhs->result << ", " << (is_and ? rhs_label : end_label) << ", " << (is_and ? end_label : rhs_label) << "\n";

            //这段在文本级虽然会出现，但是在内存级的ir的时候，因为上一条是br指令，所以这一基本块可能会被跳过？
            cout <<rhs_label << ":\n";
            rhs->Dump();
            string rhs_bool =EmitBinary("ne",rhs->result,"0");
            cout << " store " << rhs_bool << ", " << addr << "\n";
            cout << " jump " << end_label << "\n";

            cout << end_label << ":\n";
            result =NewTemp();
            cout << " " << result << " = load " << addr << "\n";
            return ;
        }

        rhs->Dump();
        string koopa_op;
        if (op == "+") {
            koopa_op = "add";
        } else if (op == "-") {
            koopa_op = "sub";
        } else if (op == "*") {
            koopa_op = "mul";
        } else if (op == "/") {
            koopa_op = "div";
        } else if (op == "%") {
            koopa_op = "mod";
        } else if (op == "<") {
            koopa_op = "lt";
        } else if (op == ">") {
            koopa_op = "gt";
        } else if (op == "<=") {
            koopa_op = "le";
        } else if (op == ">=") {
            koopa_op = "ge";
        } else if (op=="=="){
            koopa_op="eq";
        } else if (op=="!="){
            koopa_op="ne";
        } else {
            throw logic_error("未知的二元运算符: " + op);
        }
        result = EmitBinary(koopa_op,lhs->result,rhs->result);
        
    }

    int Calc() const override {
        int left = lhs->Calc();

        //这里是保持短路求值？？
        if (op=="&&")
            return left!=0&&rhs->Calc()!=0;
        if (op=="||")
            return left!=0||rhs->Calc()!=0;

        int right =rhs->Calc();

        if (op=="+") return left+right;
        if (op=="-") return left-right;
        if (op=="*") return left*right;

        if (op=="/" || op=="%"){
            if (right==0){
                throw runtime_error("常量表达式的除数为0!!");
            }
            return op=="/" ? left/right : left%right;
        }

        if (op == "<")  return left < right;
        if (op == ">")  return left > right;
        if (op == "<=") return left <= right;
        if (op == ">=") return left >= right;
        if (op == "==") return left == right;
        if (op == "!=") return left != right;

        throw logic_error("未知的二元运算符" + op);
    }
};

//名字引用节点，需要的时候就可以来查表
class LValAst :public BaseAst{
public:
    string ident;
    int Calc() const override {
        return LookupConst(ident);
    }

    void Dump() const override {
        const auto &symbol=LookupSymbol(ident);
        //查表，查到了就用resuct字符串记录"7"
        if (symbol.kind==SymbolKind::Constant){
            result=to_string(symbol.const_value);
        } else {
            result=NewTemp();
            cout << " " << result << " = load " << symbol.addr << "\n";
        }

    }
};


//一个常量定义 a = 1 + 2 这样子
class ConstDefAst : public BaseAst{
public:
    string ident;
    unique_ptr<BaseAst> init;

    void Dump() const override {
        auto & table = SymbolTable();

        if (table.find(ident)!=table.end()){
            throw runtime_error("常量重复定义: "+ ident);
        }

        int value = init->Calc();//因为多态，这里用的二员运算的calc
        table.emplace(
            ident,
            SymbolInfo{SymbolKind::Constant,value,""}//这是c++17的语法，聚合初始化
        );
    }
};


//一条常量声明 表示 cosnt int a,b;
class ConstDeclAst :public BaseAst{
public:
    vector<unique_ptr<BaseAst>> defs;
    void Dump() const override {
        for (const auto & def :defs){
            def->Dump();
        }
    }

};


//一个变量定义
class VarDefAst : public BaseAst{
public:
    string ident;
    unique_ptr<BaseAst> init;
    void Dump() const override {
        auto &table =SymbolTable();
    
        if (table.find(ident)!=table.end()){
            throw runtime_error("标识符重复定义: " + ident);
        }
        //变量要先搓出ir表
        string addr = NewVarAddr(ident);//两个作用域同名的a 可以变成var a0 a1这样子。
        cout<< " " << addr << " = alloc i32\n";
        table.emplace(
            ident,
            SymbolInfo{SymbolKind::Variable,0,addr}
        );

        if (init) {//然后打印一个存储位置的ir ，注意这个0是存储位置，所以无意义。
            init->Dump();
            cout << " store " << init->result << ", " << addr <<"\n";
        }
    }
};

//一条变量声明
class VarDeclAst : public BaseAst{
public:
    vector<unique_ptr<BaseAst>> defs;

    void Dump() const override {

        for (const auto & def :defs){
            def->Dump();
        }
    }
};



class AssignStmtAst : public BaseAst {
public:
    string ident;
    unique_ptr<BaseAst> expr;

    void Dump() const override {
        const auto & symbol=LookupSymbol(ident);

        if (symbol.kind != SymbolKind::Variable) {
            throw runtime_error ("不能给常量复制 : " + ident);
        }

        expr->Dump();
        cout<< " store " <<expr->result << ", " << symbol.addr << "\n";
    }
};


class WhileStmtAst : public BaseAst {
public:
    unique_ptr<BaseAst> cond;
    unique_ptr<BaseAst> body;


    void Dump () const override {
        string cond_label = NewLabel("while_cond");
        string body_label = NewLabel ("while_body");
        string end_label = NewLabel("while_end");

        //1.从当前的基本块进入条件块
        cout << " jump " << cond_label << "\n";

        //2.每次到条件块，都重新计算条件
        cout << cond_label << ":\n";
        cond->Dump();
        cout << " br " << cond->result << ", " << body_label << ", " << end_label << "\n";

        //3.根据br的条件，如果是非零的话，就执行
        cout << body_label << ":\n";
        LoopStack().push_back(LoopInfo{cond_label,end_label});
        body->Dump();
        LoopStack().pop_back();

        //4.循环体正常结束，回到条件块
        if (!body->IsTerminated()) {
            cout << " jump " << cond_label << "\n";
        }

        //5.条件是0 从这里继续执行
        cout << end_label << ":\n";
    }



};


class BreakStmtAst : public BaseAst {
public:
    void Dump() const override {
        const auto & loops = LoopStack(); //vector
        if (loops.empty()) {
            throw runtime_error ("break 只能出现在循环内");
        }
        cout << " jump " << loops.back().end_label << "\n";
    }

    bool IsTerminated() const override {
        return true;
    }
};


class ContinueStmtAst : public BaseAst  {
public:
    void Dump () const override {
        const auto &loops = LoopStack();
        if (loops.empty()) {
            throw runtime_error ("continue 只能出现在循环内");
        }

        cout << " jump " << loops.back().cond_label << "\n";
    }

    bool IsTerminated () const override {
        return true;
    }
};