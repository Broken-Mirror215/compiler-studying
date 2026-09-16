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



inline unordered_map<string,SymbolInfo> & SymbolTable(){
    static unordered_map<string,SymbolInfo> table;
    return table;
}

//查名字要求返回的完整的符号信息。
inline const SymbolInfo& LookupSymbol(const string & name){
    const auto & table= SymbolTable();
    auto it = table.find(name);
    if (it==table.end()){
        throw runtime_error("使用了未定义的标识符： "+name);
    }

    return it->second;
}

//查名字，并且要求他必须是常量，然后返回一个整数值。
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

    virtual int Calc() const {
        throw logic_error("这个Ast节点不能作为常量表达式求值!!");
    }
};

class CompUnitAst : public BaseAst{
public:
    unique_ptr<BaseAst> FuncDef;   
    // void Dump() const override {
    //     cout << "CompUnitAST { ";
    //     FuncDef->Dump();
    //     cout << " }";
    // }

    void Dump() const override{
        //每次处理就清空一下符号表。
        SymbolTable().clear();
        FuncDef->Dump();
    }
};

class FuncDefAst : public BaseAst{
public:
    unique_ptr<BaseAst> func_type;
    string ident;
    unique_ptr<BaseAst> block;
    // void Dump() const override {
    //     cout << "FuncDefAST { ";
    //     func_type->Dump();
    //     cout << ", " << ident << ", ";
    //     block->Dump();
    //     cout << " }";
    // }

    void Dump() const override{
        cout << "fun @" << ident << "(): ";
        func_type->Dump();
        cout << " {\n";                                                                block->Dump();
        cout << "}\n";
    }
};

 class FuncTypeAst : public BaseAst {
   public:
    // void Dump() const override {
    //     cout << "FuncTypeAST { int }";
    // }

    void Dump() const override {
        cout<<"i32";
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
    

    //ds 4 flash 3.1
    void Dump() const override{
        expr->Dump();
        cout << " ret " << expr->result <<"\n";
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


    vector<unique_ptr<BaseAst>> stmts;

    void Dump() const override {
        cout <<"%entry:\n";
        
        for (auto & stmt:stmts){
            stmt->Dump();

             if (dynamic_cast<StmtAst*>(stmt.get())!=nullptr){
                break;
             }
        }

       

    }
};


class NumberAst : public BaseAst{
public:
    int number=0;
    //ds 4 flash 3.1
    void Dump() const override{
        result=to_string(number);
    }

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
        rhs->Dump();
        string koopa_op;

        if (op=="&&"|| op=="||"){
            string left_bool =EmitBinary("ne",lhs->result,"0");
            string right_bool = EmitBinary("ne",rhs->result,"0");
            string logical_op = (op=="&&")? "and" : "or";
            result=EmitBinary(logical_op,left_bool,right_bool);
            return ;
        }

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

        int value = init->Calc();
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

        string addr = "%var_" + ident;
        cout<< " " << addr << " = alloc i32\n";
        table.emplace(
            ident,
            SymbolInfo{SymbolKind::Variable,0,addr}
        );

        if (init) {
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

