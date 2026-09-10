#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "../AST/BaseAst.h"
#include <sstream>
#include "koopa.h"

using namespace std;

extern FILE* yyin;
extern int yyparse(unique_ptr<BaseAst>&ast);

int main(int argc,const char* argv[]){


  assert(argc==5);
  auto mode=argv[1];
  auto input=argv[2];
  auto output=argv[4];

  yyin=fopen(input,"r");
  assert(yyin);


  unique_ptr<BaseAst> ast;
  auto ret=yyparse(ast);
  assert(!ret);


  if (string(mode) != "-koopa" || string(argv[3]) != "-o") {
      cerr << "用法: compiler -koopa 输入文件 -o 输出文件\n";
      return 1;
  }

  //1.把dump 输出的koopa ir 收集到字符串流
  stringstream ir_stream;
  auto *old_buffer=cout.rdbuf(ir_stream.rdbuf());
  //这行会临时改变cout的位置，重定向到ir_stream
  koopa_program_t program; //这个变量的作用是用来解析结果的。

  ast->Dump();
  cout.rdbuf(old_buffer);
  string ir=ir_stream.str();

  //2.解析文本的Koopa IR
  auto error=koopa_parse_from_string(ir.c_str(),&program);
  if (error!=KOOPA_EC_SUCCESS){
    cerr<<"Koopa IR 解析失败"<<"\n";
    return 1;
  }

  //3.构建cpp可以访问的raw program
  koopa_raw_program_builder_t builder=koopa_new_raw_program_builder();
  koopa_raw_program_t raw=koopa_build_raw_program(builder,program);
  //这上面两个变量是被封装起来的数据结构 builder是负责构建raw program的，并管理构建过程中分配的内存
  //raw是构建完成后，供你访问的程序结构。

  //已经构建好的raw,可以释放program
  koopa_delete_program(program);

  //4.先读取函数数量，验证我们确实拿到了数据结构
  cerr<<"IR中的函数数量: "<<raw.funcs.len<<"\n";

  //后续读取函数、基本块、指令的代码放在这里。

  //必须要等raw使用完毕，才能放builder
  koopa_delete_raw_program_builder(builder);

  //5.仍然用原来的koopa 输出功能
  if (!freopen(output,"w",stdout)){
    perror("打开输出文件失败");//向标准错误打印
    return 1;
  }


  cout<<ir;
  return 0;

}
