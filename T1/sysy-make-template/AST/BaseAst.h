#pragma once
#include <memory>
#include <string>
#include <iostream>
using namespace std;
struct BaseAst{
public:
    virtual ~BaseAst()=default; //析构函数怎么设置成虚函数了？
};

class CompUnitAst : public BaseAst{
public:
    unique_ptr<BaseAst> FuncDef;   
};

class FuncDefAst : public BaseAst{
public:
    unique_ptr<BaseAst> func_type;
    string ident;
    unique_ptr<BaseAst> block;
};

 class FuncTypeAst : public BaseAst {
   public:
    
};

class BlockAst : public BaseAst {
   public:
    unique_ptr<BaseAst> stmt;
};

class StmtAst : public BaseAst {
   public:
    int number = 0;
};



