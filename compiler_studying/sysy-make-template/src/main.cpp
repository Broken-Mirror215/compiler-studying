#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "../AST/BaseAst.h"
#include <sstream>
#include "koopa.h"
#include <unordered_map>
#include <stdexcept>

using namespace std;

extern FILE* yyin;
extern int yyparse(unique_ptr<BaseAst>&ast);

void LoadValue(koopa_raw_value_t value,const string &reg,const unordered_map<koopa_raw_value_t,string> &value_regs){
  if (value->kind.tag==KOOPA_RVT_INTEGER){
    cout << " li "<< reg <<", " << value->kind.data.integer.value << "\n";
  }
  else
  {
    cout << " mv " << reg << ", " << value_regs.at(value) << "\n";
  }
}

void GenRiscV(const koopa_raw_program_t &raw){
  cout<<" .text\n";//告诉汇编器，这后面是属于代码段的
  
  assert(raw.funcs.kind==KOOPA_RSIK_FUNCTION);

  for (size_t i=0;i<raw.funcs.len;i++){ //这是在找到程序里面的每一个函数...
    auto func=reinterpret_cast<koopa_raw_function_t>(raw.funcs.buffer[i]);
   

    string name=func->name;//我将得到@main
    name=name.substr(1);//变成main
    cout<<" .global "<< name<< "\n";//写一个.global name
    cout<< name << ":\n";

    // 每个函数单独记录 Koopa 运算结果所在的寄存器。
    unordered_map<koopa_raw_value_t, string> value_regs;
    const string result_regs[]={"t2","t3","t4","t5","t6","a1","a2","a3","a4","a5","a6","a7"};
    size_t next_reg=0;

    //---根据这上面的打印内容，我给risc-v声明了函数符号与入口的标记。

    
    assert(func->bbs.kind==KOOPA_RSIK_BASIC_BLOCK);
    
    for (size_t j=0;j<func->bbs.len;j++){ //进入这个函数的基本块
      auto bb=reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[j]);
      assert(bb->insts.kind==KOOPA_RSIK_VALUE);

     
      for (size_t k=0; k <bb->insts.len;k++){  //这个就是进入到基本块读取指令
        auto inst=reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[k]);

        
        switch (inst->kind.tag){
          case KOOPA_RVT_BINARY :{
            const auto & binary=inst->kind.data.binary;
            const size_t reg_count=sizeof(result_regs)/sizeof(result_regs[0]);

            if (next_reg>=reg_count){
              throw runtime_error("临时寄存器不足！！！！");
            }

            string dest=result_regs[next_reg++];



            // 从独立的结果寄存器或立即数加载两个操作数。
            LoadValue(binary.rhs,"t1",value_regs);
            LoadValue(binary.lhs,"t0",value_regs);

            switch (binary.op)
            {
            case KOOPA_RBO_ADD:
              cout<<" add " << dest <<" , t0, t1\n";
              break;
            case KOOPA_RBO_SUB:
              cout<< " sub " << dest << ", t0, t1\n";
              break;
            case KOOPA_RBO_MUL:
              cout<< " mul " << dest <<", t0, t1\n";
              break;
            case KOOPA_RBO_DIV:
              cout << " div "<< dest <<", t0, t1\n";
              break;
            case KOOPA_RBO_MOD:
              cout << " rem " << dest <<", t0, t1\n";
              break;
            case KOOPA_RBO_EQ:
              cout<< " xor " << dest <<", t0, t1\n";
              cout<<" seqz "<< dest << ", "<< dest << "\n";
              break;
            case KOOPA_RBO_NOT_EQ:
              cout << " xor " << dest << ", t0, t1\n";
              cout << " snez " << dest << ", " << dest << "\n";
              break;
            case KOOPA_RBO_LT:
              cout << " slt " << dest << ", t0, t1\n";
              break;
            case KOOPA_RBO_GT:
              cout << " slt " << dest << ", t1, t0\n";
              break;
            case KOOPA_RBO_LE:
              // a <= b 等价于 !(b < a)。
              cout << " slt " << dest << ", t1, t0\n";
              cout << " seqz " << dest << ", " << dest << "\n";
              break;
            case KOOPA_RBO_GE:
              // a >= b 等价于 !(a < b)。
              cout << " slt " << dest << ", t0, t1\n";
              cout << " seqz " << dest << ", " << dest << "\n";
              break;
            case KOOPA_RBO_AND:
              cout << " and " << dest << ", t0, t1\n";
              break;
            case KOOPA_RBO_OR:
              cout << " or " << dest << ", t0, t1\n";
              break;
            
            default: 
              cerr << "未支持这个二元运算\n";
              return;
            }
            value_regs[inst]=dest;
            break;
          }

          case KOOPA_RVT_RETURN : {
            auto ret_value =inst->kind.data.ret.value;
            assert(ret_value);

            LoadValue(ret_value,"a0",value_regs);
            cout << " ret\n";
            break;
          }

          default:
            cerr << "暂未支持这个指令类型\n";
            return;
        }
      }
    }
  }
}

//也就是说这么长一串代码，只是为了拿return 后面的整数....


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


  if (string(mode) != "-koopa"&& string(mode) != "-riscv" || string(argv[3]) != "-o") {
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


  //5.仍然用原来的koopa 输出功能
  if (!freopen(output,"w",stdout)){
    perror("打开输出文件失败");//向标准错误打印
    return 1;
  }

  if (string(mode)=="-koopa"){
    cout<<ir;
  }
  else
  {
    GenRiscV(raw);
  }

  koopa_delete_raw_program_builder(builder);
  return 0;

}
