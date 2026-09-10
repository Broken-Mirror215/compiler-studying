#pragma once
#include <memory>
#include <string>
#include <iostream>
using namespace std;
struct BaseAst{
public:
    virtual ~BaseAst()=default; //析构函数怎么设置成虚函数了？
    virtual void Dump() const = 0;
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
        cout <<"%enrty:\n";
        stmt->Dump();
    }
};

class StmtAst : public BaseAst {
   public:
    int number = 0;
    // void Dump() const override {
    //     cout << "StmtAST { " << number << " }";
    // }

    void Dump() const override{
        cout<<" ret "<< number <<"\n";
    }
};


