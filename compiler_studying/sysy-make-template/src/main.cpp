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

struct StackFrame{
  unordered_map<koopa_raw_value_t,int> offsets;
  int size=0;
};

StackFrame BuildStackFrame(koopa_raw_function_t func) {
  StackFrame frame;
  int next_offset =0 ;

  assert (func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);

  for (size_t i=0;i<func->bbs.len;++i){
    auto bb = reinterpret_cast<koopa_raw_basic_block_t> (func->bbs.buffer[i]);
    
    assert (bb->insts.kind==KOOPA_RSIK_VALUE);
    for (size_t j = 0; j<bb->insts.len;++j){
      auto inst = reinterpret_cast <koopa_raw_value_t>(bb->insts.buffer[j]);

      if (inst->ty->tag==KOOPA_RTT_UNIT){
        continue;
      }

      //当前阶段
      //alloc i32 对应是4字节
      //load ,binary 的i32结果也是4字节
      frame.offsets.emplace(inst,next_offset);
      next_offset+=4;
    }
  }

  frame.size = (next_offset+15)/16 *16;
  return frame;
  
}

void AdjustStack(int delta) {
  if (delta==0){
    return ;
  }

  if (delta >=-2048 &&delta<=2047) {
    cout << " addi sp ,sp, " << delta << "\n";
  }else {
    cout<< " li t6, " << delta << "\n";
    cout<< " add sp, sp, t6\n";
  }
}

void LoadStack(const string & reg , int offset) {


  assert(reg!="t6");

  if (offset>=-2048&&offset<=2047) {
    cout << " lw "<< reg << ", " << offset << "(sp)\n";
  }else 
  {
    cout<< " li t6, " <<offset << "\n";
    cout<< " add t6, sp, t6\n";
    cout<<" lw " << reg << ", 0(t6)\n";
  }
}

void StoreStack(const string & reg,int offset) {
  assert (reg != "t6");

  if (offset >=-2048 && offset <=2047) {
    cout<< " sw " << reg << ", " << offset << "(sp)\n";
  } else {
    cout<< " li t6, " << offset << "\n";
    cout << "add t6, sp, t6\n";
    cout<< " sw " <<reg << ", 0(t6)\n";
  }
}

//整数就用li 其他从栈读取
void LoadValue(koopa_raw_value_t value,const string &reg,const StackFrame & frame){
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
      cout << " li " << reg << ", " << value->kind.data.integer.value << "\n";
    } else 
    {
      assert(value->ty->tag==KOOPA_RTT_INT32);
      LoadStack(reg,frame.offsets.at(value));
    }
}

void GenRiscV(const koopa_raw_program_t &raw){
  cout<<" .text\n";//告诉汇编器，这后面是属于代码段的
  
  assert(raw.funcs.kind==KOOPA_RSIK_FUNCTION);

  for (size_t i=0;i<raw.funcs.len;i++){ //这是在找到程序里面的每一个函数...
    auto func=reinterpret_cast<koopa_raw_function_t>(raw.funcs.buffer[i]);
    StackFrame frame = BuildStackFrame(func);
    cerr << "函数 " << func->name << " 的栈帧大小：" << frame.size << " 字节\n";

    string name=func->name;//我将得到@main
    name=name.substr(1);//变成main
    cout<<" .global "<< name<< "\n";//写一个.global name
    cout<< name << ":\n";
    AdjustStack(-frame.size); //每个函数都有一个自己的栈

    // // 每个函数单独记录 Koopa 运算结果所在的寄存器。
    // unordered_map<koopa_raw_value_t, string> value_regs;  //这个是记录某个koopa运算放在哪个寄存器里面，比如这个%0 
    // const string result_regs[]={"t2","t3","t4","t5","t6","a1","a2","a3","a4","a5","a6","a7"};
    // size_t next_reg=0;

    //---根据这上面的打印内容，我给risc-v声明了函数符号与入口的标记。

    
    assert(func->bbs.kind==KOOPA_RSIK_BASIC_BLOCK);
    
    for (size_t j=0;j<func->bbs.len;j++){ //进入这个函数的基本块
      auto bb=reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[j]);
      assert(bb->insts.kind==KOOPA_RSIK_VALUE);

     
      for (size_t k=0; k <bb->insts.len;k++){  //这个就是进入到基本块读取指令
        //一条条koopa ir
        auto inst=reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[k]);

        //这是先判断哪种指令
        switch (inst->kind.tag){
          case KOOPA_RVT_BINARY :{
            const auto & binary=inst->kind.data.binary; //这个是二元运算的数据
            
            // 从独立的结果寄存器或立即数加载两个操作数。
            LoadValue(binary.lhs,"t0",frame);
            LoadValue(binary.rhs,"t1",frame);
            string dest ="t0";
            

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
            StoreStack(dest,frame.offsets.at(inst));//xxx.at是记录这个元素的在栈内的偏移量。
            break;
          }

          case KOOPA_RVT_RETURN : {
            auto ret_value =inst->kind.data.ret.value; //return 的数据
            assert(ret_value);

            LoadValue(ret_value,"a0",frame);
            AdjustStack(frame.size);
            cout << " ret\n";
            break;
          }
          case KOOPA_RVT_ALLOC :{

            //buildStackFram为这个局部变量留好了空间
            //函数入口统一成sp,此处不要生成指令
            break;
          }
          case KOOPA_RVT_LOAD :{
            const auto & load =inst->kind.data.load;
            assert(load.src->kind.tag==KOOPA_RVT_ALLOC);
            LoadStack("t0",frame.offsets.at(load.src));
            StoreStack("t0",frame.offsets.at(inst));
            break;
          }
          case KOOPA_RVT_STORE: {
            const auto & store =inst->kind.data.store;
            assert(store.dest->kind.tag ==KOOPA_RVT_ALLOC);
            LoadValue(store.value,"t0",frame);
            StoreStack("t0",frame.offsets.at(store.dest));
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



int main(int argc,const char* argv[]){


  assert(argc==5);
  auto mode=argv[1];
  auto input=argv[2];
  auto output=argv[4];

  yyin=fopen(input,"r");
  assert(yyin);


  unique_ptr<BaseAst> ast;
  auto ret=yyparse(ast);//解析器按照我的bison语法去建树了。
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
