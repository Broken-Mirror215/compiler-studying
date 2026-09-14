#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <stdexcept>
using namespace std;
//ds v4 flash 3.1
inline string NewTemp(){
    static int cnt=0;
    return "%" + to_string(cnt++);
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

class BlockAst : public BaseAst {
   public:
    unique_ptr<BaseAst> stmt;
    // void Dump() const override {
    //     cout << "BlockAST { ";
    //     stmt->Dump();
    //     cout << " }";
    // }

    void Dump() const override {
        cout <<"%entry:\n";
        stmt->Dump();
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

class NumberAst : public BaseAst{
public:
    int number=0;
    //ds 4 flash 3.1
    void Dump() const override{
        result=to_string(number);
    }
};

//二元运算模拟一元运算
class UnaryExpAst : public BaseAst{
public:
    char op;
    unique_ptr<BaseAst> operand;

    //ds 4 flash 3.1
    void Dump() const override{
        operand->Dump();
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


};
