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

inline string NewGlobalAddr (const string & ident){
    static int cnt=0;
    return "@global_" + ident + "_" + to_string(cnt++);
}



//基本块命名函数
inline string NewLabel (const string & kind) {
    static int cnt=0;
    return "%bb_" + kind + "_" + to_string(cnt++); 
}

enum class SymbolKind {
    Constant,
    Variable,
    Array,
    ConstArray,
    ArrayParam
};


class SymbolInfo {
public:
    SymbolKind kind;
    int const_value = 0;//常量使用这个字段
    string addr; //变量使用这个字段 保存koopa存储位置的数字 例如 "@x"
    size_t array_rank=0;//这个记录数组的维度
};



struct FuncInfo {
    bool is_void;
    size_t param_count;
};

inline unordered_map<string,FuncInfo>& FunctionTable() {
    static unordered_map<string,FuncInfo> functions;
    return functions;
}

inline void RegisterLibraryFunctions() {
    auto & functions = FunctionTable();
    functions.emplace("getint",FuncInfo{false,0});
    functions.emplace("getch",FuncInfo{false,0});
    functions.emplace("getarray",FuncInfo{false,1});
    
    functions.emplace("putint",FuncInfo{true,1});
    functions.emplace("putch",FuncInfo{true,1});
    functions.emplace("putarray",FuncInfo{true,2});

    functions.emplace("starttime",FuncInfo{true,0});
    functions.emplace("stoptime",FuncInfo{true,0});

    
}
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
    static vector<unordered_map<string,SymbolInfo>> scopes; //基本块里面的符号表
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


//lab 9.3  单个形参
class FuncFParamAst : public BaseAst {
public:
    string ident;
    bool is_array = false;

    vector<unique_ptr<BaseAst>> array_dims;

    string KoopaType() const;

    void Dump() const override {}


};



//这是生成ir的入口。
class CompUnitAst : public BaseAst{
public:
    //一串函数
    vector<unique_ptr<BaseAst>> items;
    // void Dump() const override {
    //     cout << "CompUnitAST { ";
    //     FuncDef->Dump();
    //     cout << " }";
    // }

    void Dump() const override{
        //每次处理就清空一下符号表。
        ScopeStack().clear();
        LoopStack().clear();
        FunctionTable().clear();
        RegisterLibraryFunctions();
        EnterScope();//这就是全局的作用域

        cout << "decl @getint(): i32\n";
        cout << "decl @getch(): i32\n";
        cout << "decl @getarray(*i32): i32\n";
        cout << "decl @putint(i32)\n";
        cout << "decl @putch(i32)\n";
        cout << "decl @putarray(i32,*i32)\n";
        cout << "decl @starttime()\n";
        cout << "decl @stoptime()\n\n";

        for (auto & item :items) {
            item->Dump();
        }
        ExitScope();
    }
};

class FuncDefAst : public BaseAst{
public:
    unique_ptr<BaseAst> func_type;
    string ident;
    unique_ptr<BaseAst> block;
    //存形参名字
    vector<unique_ptr<FuncFParamAst>> params;
    // void Dump() const override {
    //     cout << "FuncDefAST { ";
    //     func_type->Dump();
    //     cout << ", " << ident << ", ";
    //     block->Dump();
    //     cout << " }";
    // }

    //lab 8.1
    void Dump() const override;
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
    vector<unique_ptr<FuncFParamAst>> params;
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
    void Dump () const override {
        //查找被调用函数
        const auto& functions = FunctionTable();
        auto it = functions.find(ident);
        if (it == functions.end()) {
            throw runtime_error ("调用了未定义的函数: " + ident);
        }
        const auto & info =it->second;

        if (args.size()!=info.param_count) {
            throw runtime_error("函数实参数量不匹配: " + ident);
        }

        //先生成所有实参的求值指令
        vector<string>values;
        for (const auto& arg: args) {
            arg->Dump();

            if (arg->result.empty()) {
                throw runtime_error ("实参不能是 void 表达式: " + ident);
            }

            values.push_back(arg->result);
        }

        result.clear();

        if (info.is_void) {
            cout << " call @" << ident << "(";
        } else {
            result = NewTemp();
            cout << " " << result << " = call @" <<ident << "(";
        }

        //输出已经求好的实参，结束调用指令
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) cout << ", ";
            cout << values[i];
        }
        cout << ")\n";
    }
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

    //普通语句嵌套语句块：创建自己的作用域。
    void Dump() const override {
        EnterScope();
        DumpInCurrentScope();
        ExitScope();
    }

    //这里为了只处理块里面的语句，使用已经存在的作用域
    void DumpInCurrentScope() const {
        terminater=false;
        for (auto & stmt:stmts){
            stmt->Dump();

             if (stmt->IsTerminated()){
                terminater=true;
                break;
             }
        }       
    }

    bool IsTerminated () const override {
        return terminater;
    }
};



inline void FuncDefAst::Dump() const {
    //开始生成一个程序时，重新建立函数的信息
    auto * type = static_cast<FuncTypeAst*>(func_type.get());
    auto& functions = FunctionTable();

    if (functions.find(ident) != functions.end() || SymbolTable().count(ident)) {
        throw runtime_error("函数重复定义： " + ident);  
    }

    functions.emplace(
        ident,
        FuncInfo({type->is_void,params.size()})  
    );

    func_type->Dump();
    cout << "fun @" << ident << "(";

    for (size_t i=0;i<params.size();i++){
        if (i>0) cout << ", ";
        cout << "@" << params[i]->ident << ": " << params[i]->KoopaType();
    }
    cout << ")" << func_type->result << " {\n";
    cout << "%entry:\n";

    //形参和函数体最外层用这个栈
    EnterScope();

    //然后把每个形参变成可读写的局部变量
    for (const auto & param: params) {
       const string & name = param->ident;
       auto & table = SymbolTable();

       if (table.find(name) != table.end()) {
        throw runtime_error("形参重复定义: " + name);
       }

       string addr = NewVarAddr(name);
       string param_type = param->KoopaType();

       cout << " " << addr << " = alloc " << param_type << "\n";
       cout << " store @" << name << ", " << addr << "\n";

       SymbolKind kind = param->is_array ? SymbolKind::ArrayParam : SymbolKind::Variable;

       size_t rank = param->is_array ? param->array_dims.size()+1 : 0;

       table.emplace(name,SymbolInfo{kind,0,addr,rank});
    }

    //在刚才作用域生成函数体
    auto * body = static_cast<BlockAst*>(block.get());
    body->DumpInCurrentScope();

    if (type->is_void&&!body->IsTerminated()) {
        cout <<" ret\n";
    }

    ExitScope();
    cout << "}\n";
}

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


class ArrayDimsAst : public BaseAst {
public:
    vector<unique_ptr<BaseAst>> dims;

    void Dump() const override {
        throw logic_error ("维度列表由数组定义节点处理");
    }
};

//把各个维度的常量表达式算长度
inline vector<int> CalcArrayDims(const vector<unique_ptr<BaseAst>>& dims){

    vector<int>lengths;
    
    for (const auto & dim : dims) {
        int len = dim->Calc();
        if (len<=0) {
            throw runtime_error("数组长度必须大于0");
        }
        lengths.push_back(len);
    }
    return lengths;
}

//根据各维构造koopa数据类型
inline string ArrayType (const vector<int>&lengths) {
    string type = "i32";

    for (auto it=lengths.rbegin();it!=lengths.rend();it++) {
        type = "[" + type + ", " + to_string(*it) + "]";
    }

    return type;
}

inline string FuncFParamAst::KoopaType() const {
    if (!is_array) {
        return "i32";
    }

    auto lengths = CalcArrayDims(array_dims);
    return "*" + ArrayType(lengths);
}




//名字引用节点，需要的时候就可以来查表
class LValAst :public BaseAst{
public:

    string ident;
    //先算出左值地址，然后读取。
    vector<unique_ptr<BaseAst>> indices;
    string Address(bool for_write = false) const {
        const auto &symbol = LookupSymbol(ident);
    
        if (indices.empty()&&symbol.kind==SymbolKind::Variable) {
            return symbol.addr;
        }


        bool is_param = symbol.kind == SymbolKind::ArrayParam;

        //有下标必须是普通数组、常量数组或数组参数
        if (symbol.kind != SymbolKind::Array &&
            symbol.kind != SymbolKind::ConstArray &&
            !is_param) {
            throw runtime_error ("不是数组: " + ident);
        }
        if (for_write && symbol.kind == SymbolKind::ConstArray) {
            throw runtime_error("不能修改常量数组: " + ident);
        }

        if (indices.size() > symbol.array_rank) {
            throw runtime_error("数组下标数量过多" + ident);
        }

        if (for_write&&indices.size() != symbol.array_rank) {
            throw runtime_error("不能给整个数组或者子数组赋值： " + ident);
        }



        string addr =symbol.addr;

        if (is_param ) {
            string ptr = NewTemp();
            cout << " " << ptr << " = load " << addr << "\n";
            addr = ptr;
        }
        for (size_t i = 0; i< indices.size(); i ++) {
            indices[i]->Dump();
            string ptr = NewTemp();

            string op =(is_param&&i==0) ? "getptr" : "getelemptr";
            cout << " " << ptr << " = " << op << " " << addr << ", " << indices[i]->result << "\n";
            addr = ptr;
        }
        return addr;
    }

    int Calc() const override {
        if (!indices.empty()) throw runtime_error("不能在常量表达式中求数组元素");
        return LookupConst(ident);
    }

    void Dump() const override {
        const auto &symbol=LookupSymbol(ident);
        //查表，查到了就用resuct字符串记录"7"
        if (indices.empty()&&symbol.kind==SymbolKind::Constant){
            result=to_string(symbol.const_value);
            return ;
        } 
        string addr =Address();
        
        bool is_array = symbol.kind == SymbolKind::Array || symbol.kind == SymbolKind::ConstArray || symbol.kind== SymbolKind::ArrayParam;
        
        if (is_array&&indices.size() < symbol.array_rank) {
            
            if (symbol.kind== SymbolKind::ArrayParam&&indices.empty()) {
                result = addr;
                return ;
            }

            result = NewTemp();
            cout << " " << result << " = getelemptr " << addr << ", 0\n";
            return; 
        }

        // 标量变量或完整数组元素：读取整数
        result = NewTemp();
        cout << " " << result << " = load " << addr << "\n";
    }
};


class ArrayInitAst : public BaseAst {
public:
    vector<unique_ptr<BaseAst>> elems;

    void Dump() const override {
        throw logic_error("数组初始化列表要由数组定义节点处理");
    }
};

//depth当前列表初始化从哪一维开始的数组
//start这个数组在展平结果中的起点
//sizes[d] 是从d维开始的数组，共有多少个整数元素
inline void FillArrayInit(const ArrayInitAst& list,size_t depth,size_t start,const vector<size_t>& sizes,vector<const BaseAst*>&flat) {


    size_t pos = 0 ;
    size_t capacity =sizes[depth];
    size_t rank = sizes.size()-1;


    for (const auto& elem:list.elems) {
        if (pos>=capacity) {
            throw runtime_error("数组初始值多于当前子数组容量");
        }

        auto*child = dynamic_cast<const ArrayInitAst*>(elem.get());
        if (!child) {
            //表达式占一个整数位置
            flat[start+pos] = elem.get();
            ++pos;
            continue;
        }

        size_t child_depth = depth + 1;
        while (child_depth < rank&& pos%sizes[child_depth] != 0) {
            child_depth++;
        }

        if (child_depth == rank) {
            throw runtime_error("嵌套初始化列表没有对齐数组边界");
        }
    
        FillArrayInit(*child,child_depth,start+pos,sizes,flat);

        pos+=sizes[child_depth];
    
    }
}

inline vector<const BaseAst*> FlattenArrayInit(const BaseAst&init,const vector<int>& lengths) {
    auto * list =dynamic_cast<const ArrayInitAst*>(&init);
    if (!list) {
        throw runtime_error("数组要初始化列表");
    }

    if (lengths.empty()) {
        throw logic_error("整理数组初值的时候没有数组维度");
    }

    vector<size_t> sizes(lengths.size()+1,1);

    for (size_t d =lengths.size();d>0;d--) {
        sizes[d-1] = static_cast<size_t>(lengths[d-1]*sizes[d]);
    }

    vector<const BaseAst*> flat(sizes[0],nullptr);
    FillArrayInit(*list,0,0,sizes,flat);
    return flat;
}


inline void EmitArrayAggregate(const vector<int>&values,const vector<int>& lengths,size_t depth,size_t & pos) {
    if (depth == lengths.size()) {
        cout << values[pos++];
        return;
    }

    cout << "{";
    for (int i=0;i <lengths[depth];i++){
        if (i > 0) cout << ", ";
        EmitArrayAggregate(values,lengths,depth+1,pos);
    }
    cout <<"}";
}

inline string ArrayElementAddress(const string&base,const vector<int>&lengths,size_t offset) {

    size_t stride =1;
    for (int len:lengths) {
        stride *=static_cast<size_t>(len);
    }
    string addr =base;
    for (int len: lengths) {

        stride /=static_cast<size_t>(len);
        size_t index = offset / stride;
        offset  %= stride;
        string ptr = NewTemp();
        cout << " " << ptr << " = getelemptr " << addr << ", " << index << "\n";
        addr = ptr;
    }

    return addr;

}
//一个常量定义 a = 1 + 2 这样子
class ConstDefAst : public BaseAst{
public:
    string ident;
    unique_ptr<BaseAst> init;
    vector<unique_ptr<BaseAst>> array_dims;//不就一个长度吗怎么还要节点,你为二维数组服务的吗？

    void Dump() const override {
        auto & table = SymbolTable();

        if (table.find(ident)!=table.end() ||
            (ScopeStack().size() == 1 && FunctionTable().count(ident))){
            throw runtime_error("常量重复定义: "+ ident);
        }

        if (array_dims.empty()) {
           int value = init->Calc();//因为多态，这里用的二员运算的calc
                table.emplace(
                ident,
                SymbolInfo{SymbolKind::Constant,value,""}//这是c++17的语法，聚合初始化
            );
            return;
        }


        auto lengths =CalcArrayDims(array_dims);
        string type =ArrayType(lengths);
        auto flat =FlattenArrayInit(*init,lengths);

        vector<int> values(flat.size(),0);
        for (size_t i = 0 ; i<flat.size();i++) {
            if (flat[i]) {
                values[i] = flat[i]->Calc();
            }
        }

        if (ScopeStack().size()==1) {
            string addr =NewGlobalAddr(ident);
            cout << "global " << addr <<" = alloc " << type << ", ";
            size_t pos =0;
            EmitArrayAggregate(values,lengths,0,pos);
            cout << "\n";

            table.emplace(ident,SymbolInfo{SymbolKind::ConstArray,0,addr,lengths.size()});
            return ;
        }

        string addr = NewVarAddr(ident);
        cout << " " << addr << " = alloc " << type << "\n";
        table.emplace(ident,SymbolInfo{SymbolKind::ConstArray,0,addr,lengths.size()});


        for (size_t i = 0; i<values.size();i++) {
            string ptr =ArrayElementAddress(addr,lengths,i);
            cout << " store " << values[i] << ", " << ptr << "\n";
        }



        // if (array_dims.size() >1) {
        //     throw runtime_error("多维数组常量初始化将在下一步实现");
        // }
        // auto lengths = CalcArrayDims(array_dims);
        // int len=lengths[0];

        // auto * list = dynamic_cast<ArrayInitAst*>(init.get());
        // if (!list) throw runtime_error ("常量数组要初始化列表");
        // if (list->elems.size()> static_cast<size_t>(len)) {
        //     throw runtime_error ("数组初始值多于数组长度");
        // }

        // vector<int>values(len,0);
        // for (size_t i=0; i <list->elems.size();i++) {
        //     values[i] = list->elems[i]->Calc();
        // }

        // if (ScopeStack().size()==1) {
        //     string addr =NewGlobalAddr(ident);
        //     cout << "global " << addr << " = alloc [i32, " << len << "], {";
        //     for (int i=0;i< len;i ++) {
        //         if (i) cout << ",";
        //         cout <<values[i];
        //     }
        //     cout << "}\n";
        //     table.emplace(ident,SymbolInfo{SymbolKind::ConstArray,0,addr,array_dims.size()});
        //     return;
        // }

        // string addr = NewVarAddr(ident);
        // cout << " " << addr << " = alloc [i32, " << len <<"]\n";
        // table.emplace(ident,SymbolInfo{SymbolKind::ConstArray,0,addr,array_dims.size()});

        // for (int i= 0;i <len;i++) {
        //     string ptr = NewTemp();
        //     cout << " " << ptr << " = getelemptr " << addr << ", " << i << "\n";
        //     cout << " store " << values[i] << ", " << ptr << "\n";
        // }
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
    vector<unique_ptr<BaseAst>> array_dims;
    void Dump() const override {
        auto &table =SymbolTable();
        if (table.count(ident) || (ScopeStack().size()==1 && FunctionTable().count(ident))) {
            throw runtime_error("标识符重定义: " + ident);
        }

        if (!array_dims.empty()) {
            auto lengths = CalcArrayDims(array_dims);
            string type =ArrayType(lengths);

            vector<const BaseAst*> flat;
           
            if (init) {
                flat = FlattenArrayInit(*init,lengths);
            }

            vector<int>values(flat.size(),0);
            if (ScopeStack().size() == 1 ) {
               
                for (size_t i = 0; i< flat.size();i++) {
                    if (flat[i]) {
                        values[i] = flat[i]->Calc();
                    }
                }

                string addr =NewGlobalAddr(ident);
                cout << "global " << addr << " = alloc " <<type << ", ";

                if (!init) {
                    cout << "zeroinit";
                }
                else
                {
                    size_t pos = 0;
                    EmitArrayAggregate(values,lengths,0,pos);
                }
                cout << "\n";
                table.emplace(ident,SymbolInfo{SymbolKind::Array,0,addr,lengths.size()});
                return;
            }
            string addr = NewVarAddr(ident);
            cout << " " << addr << " = alloc " <<type << "\n";
            table.emplace(ident,SymbolInfo{SymbolKind::Array,0,addr,lengths.size()});

            if (init) {
                for (size_t i= 0 ;i<flat.size();i++) {
                    string value = "0";

                    if (flat[i]) {
                        flat[i]->Dump();
                        value = flat[i]->result;
                    }
                    string ptr = ArrayElementAddress(addr,lengths,i);
                    cout << " store " << value << ", " << ptr << "\n";
                }
                
            }
            return ;
        }
      





        // if (!array_dims.empty()) {
        //     auto lengths = CalcArrayDims(array_dims);
        //     string type = ArrayType(lengths);

        //     if (lengths.size() > 1 && init ) {
        //         throw runtime_error ( "多维数组初始化将在下一步实现");
        //     }

        //     int len =lengths[0];
        //     auto *list = init ? dynamic_cast<ArrayInitAst*>(init.get()) : nullptr;
        //     if (init && !list) throw runtime_error("数组要初始化列表");
        //     if (list && list->elems.size() > static_cast<size_t>(len))
        //         throw runtime_error("数组初始值多于数组长度");

        //     if (ScopeStack().size() == 1) {
        //         string addr = NewGlobalAddr(ident);
        //         cout << "global " << addr << " = alloc " << type << ", ";
        //         if (!list) {
        //             cout << "zeroinit\n";
        //         } else {
        //             cout << "{";
        //             for (int i = 0; i < len; ++i) {
        //                 if (i > 0) cout << ", ";
        //                 int value = static_cast<size_t>(i) < list->elems.size()
        //                     ? list->elems[i]->Calc() : 0;
        //                 cout << value;
        //             }
        //             cout << "}\n";
        //         }
        //         table.emplace(ident, SymbolInfo{SymbolKind::Array, 0, addr,array_dims.size()});
        //         return;
        //     }

        //     // 局部数组使用 alloc，初始化时逐元素计算地址并存储。
        //     string addr = NewVarAddr(ident);
        //     cout << " " << addr << " = alloc " << type << "\n";
        //     table.emplace(ident, SymbolInfo{SymbolKind::Array, 0, addr,array_dims.size()});
        //     if (list) {
        //         for (int i = 0; i < len; ++i) {
        //             string value = "0";
        //             if (static_cast<size_t>(i) < list->elems.size()) {
        //                 list->elems[i]->Dump();
        //                 value = list->elems[i]->result;
        //             }
        //             string ptr = NewTemp();
        //             cout << " " << ptr << " = getelemptr " << addr << ", " << i << "\n";
        //             cout << " store " << value << ", " << ptr << "\n";
        //         }
        //     }
        //     return;
        // }

        if (ScopeStack().size() == 1) {
            string addr = NewGlobalAddr(ident);
            int value = init ? init->Calc() : 0;
            cout << "global " << addr << " = alloc i32, "
                 << (init ? to_string(value) : "zeroinit") << "\n";
            table.emplace(ident, SymbolInfo{SymbolKind::Variable, 0, addr});
            return;
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
    unique_ptr<LValAst> target;
    unique_ptr<BaseAst> expr;

    void Dump() const override {
        string addr =target->Address(true);
        expr->Dump();
        cout << " store " <<expr->result << ", " <<addr << "\n";
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
