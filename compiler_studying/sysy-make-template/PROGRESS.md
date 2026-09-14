# 学习进度与代码评估

更新日期：2026-09-14。

本次阅读了 Makefile、词法/语法规则、全部 AST 和入口及后端实现，并对照 HEAD 检查了未提交改动。评估范围包括 `//ds v4 flash`、`//ds 4 flash` 标记及相关的 AddExp/MulExp 规则。本次仅新增此记录，保留原实现和学习注释。

## 当前进度

**已完成到 Lv3.2 的语法分析和 Koopa IR 生成；RISC-V 后端仍处于 Lv2，尚未完成整个 Lv3。**

| 阶段 | 当前实现与验证 |
| --- | --- |
| Lv1：main 与 IR | 支持单个无参 int 函数、单条 return、整数与注释；生成并写出 Koopa IR |
| Lv2：目标代码生成 | 已通过 libkoopa 解析文本、构建 raw program，遍历函数/基本块/指令；整数返回的 RISC-V 课程测试通过 7/7 |
| Lv3.1：一元表达式 | 支持括号及一元 `+ - !`，生成正确的 Koopa IR；对应课程用例通过 |
| Lv3.2：算术表达式 | 支持二元 `+ - * / %`、优先级与左结合；对应课程用例和补充用例通过 |
| Lv3 表达式后端 | 待实现：目前不能处理 binary 指令及其结果作为 return 操作数 |
| Lv3.3：比较和逻辑表达式 | 尚未实现 `< > <= >= == != && ||` 的完整词法/语法与 IR 生成 |
| Lv4 及以后 | 尚未实现常量/变量声明、赋值、作用域、控制流、函数调用、数组等 |

这里的 `Dump()` 实际承担 IR 生成，原先的 AST 结构打印已被注释；不能把两种输出混为一谈。

## 标记代码的评估

### 1. 主要功能正确，可以作为现阶段实现保留

- `NewTemp()` 与 `EmitBinary()`（`AST/BaseAst.h:7–17`）：集中分配临时名称并输出二元指令。当前每进程编译一个函数，计数器能保证生成的临时名称不重复。
- `NumberAst::Dump()`（第 114 行）：将常量作为操作数返回，不额外输出指令，符合这里的 IR 生成需求。
- `UnaryExpAst::Dump()`（第 126 行）：先生成子表达式，`+x` 直接传递结果，`-x` 转换为 `sub 0, x`，`!x` 转换为 `eq x, 0`，语义正确。
- `BinaryExpAst::Dump()`（第 151 行）：先生成左右子树，再发射父节点指令，保证使用临时值前已经定义；五种算术运算映射正确。
- `StmtAst::Dump()`（第 104 行）：返回表达式生成的操作数，既支持立即数也支持临时值。
- `sysy.y`：`Exp → AddExp → MulExp → UnaryExp → PrimaryExp` 的分层体现优先级；AddExp/MulExp 的左递归体现左结合。括号直接传递内部表达式节点是合理的，不必为每个非终结符都建立独立 AST 类。
- 规约成功路径上，裸指针被父节点的 `unique_ptr` 接管，透传规则不会额外建立重复所有权。

例如 `return 1 + 2 * 3;` 的 AST 是 `+(1, *(2, 3))`，后序生成的核心 IR 为：

```koopa
%0 = mul 2, 3
%1 = add 1, %0
ret %1
```

### 2. 当前最重要的功能缺口：前端扩展后，后端没有跟进

位置：`src/main.cpp:45–49`。`GenRiscV()` 假定每条指令都是 return，且返回值只能是整数。

实测 `int main(){return -1;}` 和 `int main(){return 1+2;}` 在 `-riscv` 模式都触发第 45 行断言，进程收到 SIGABRT；`return 42;` 正常。新 AST 生成的是合法运算指令，错误发生在 raw Koopa IR 到 RISC-V 的阶段。

这是旧后端与新增表达式功能的衔接缺口，不是 `EmitBinary()` 输出错误。下一步需要按指令类型处理 binary/return，并记录每条运算结果的位置；仅删除断言不能解决问题。

### 3. 设计上可接受，但有后续维护成本

- `BaseAst::result`（第 25 行）采用 `mutable`，使 `Dump() const` 可以保存计算结果。当前调用顺序保证先写后读，因此本次没有发现由它导致的结果错误；不过“输出 IR”和“返回表达式的值”被隐藏在共享节点状态中。以后可以考虑让专门的表达式生成接口直接返回操作数，无须现在大改。
- `NewTemp()` 的静态计数器跨函数、跨多次生成持续增长。当前单函数、单次生成没有问题；若以后希望同一进程反复生成可重复的 IR，应把编号放入生成上下文，并明确重置边界。不能在每个表达式里重置，否则会重名。
- `UnaryExpAst` 把所有非 `+/-` 操作都当作 `!`；`BinaryExpAst` 的 default 留下空操作码后仍发射指令。当前语法限制使这些异常分支不可由合法输入触发，但以后增加操作符时应显式报错，避免静默生成错误 IR。
- Bison 的 `%union` 使用裸指针，未设置 `%destructor`。成功规约时所有权正常，但语法错误丢弃栈符号时不能自动清理这些对象。当前出错后立即终止进程，影响有限；若以后支持错误恢复或批量编译，应补充类型对应的清理策略。这也是已有解析器的工程债务。

### 4. 两处学习注释需要澄清

- `sysy.y:124`：“因为递归先处理”不够准确。**优先级由文法层次决定，计算时的先后由构造出的 AST 和后序遍历体现**。这里 AST 的算术节点确实不需要再判断优先级。
- `sysy.y:170`：“只能左递归”太绝对。准确说法是：**当前这套规约动作通过左递归自然构造左结合 AST**；其他文法配合适当建树方式或优先级声明也能实现左结合。`10-3-2` 应是 `(10-3)-2`，实测结果为 5。

AddExp 末尾缺少显式的 `;`，但当前 GNU Bison 3.8.2 接受这种省略，`-Wall -Wcounterexamples` 也未报告问题，因此不能将它列为构建失败原因。为统一书写风格，后续可补上。

## 本次验证

环境：运行中的 `sysy-dev` 课程容器，项目映射为 `/workspace/compiler_studying/sysy-make-template`。使用全新的 `/tmp/sysy-review-20260914-*` 构建目录，未依赖旧的 `build/compiler`。

| 检查 | 结果 |
| --- | --- |
| 新目录完整构建 | 成功 |
| Bison `-Wall -Wcounterexamples` | 未报告文法冲突或警告 |
| `autotest -riscv -s lv1` | 7/7 通过 |
| `autotest -koopa -s lv3` | 19/28 通过 |
| 补充 Koopa 执行测试 | 10/10 通过 |
| RISC-V 最小复现 | 整数返回成功；负号与加法表达式均触发旧后端断言 |

Lv3 失败项为 `17_lt` 至 `24_land` 以及 `27_complex_binary`，均含尚未支持的比较/逻辑运算，表现为语法分析失败。`00_pos` 至 `16_mod_neg`、`25_int_min`、`26_parentheses` 均通过。不能把 19/28 记为 Lv3 全部完成。

补充用例及期望值：

| 表达式 | 结果 |
| --- | --- |
| `1+2*3` | 7 |
| `(1+2)*3` | 9 |
| `10-3-2` | 5 |
| `24/4/2` | 3 |
| `20%6%3` | 2 |
| `(-7/3)+10` | 8 |
| `(-7%3)+10` | 9 |
| `+(- -!6)` | 0 |
| `(1+2)*(3+4)-(10-6)/2` | 19 |
| `!1+2` | 2 |

课程和补充测试均使用 autotest 验证执行结果，不仅检查 IR 字符串。补充用例保存在容器 `/tmp/sysy-review-20260914-cases`，属于临时验证材料。

构建仍有两条已有警告：`main.cpp:79` 的 `&&/||` 混用建议显式括号；`sysy.y:63` 的 `move` 建议写为 `std::move`。前者现有判断语义并未因优先级而出错，后者也不影响此次构建。

复核命令（容器内执行）：

```sh
cd /workspace/compiler_studying/sysy-make-template
make BUILD_DIR=/tmp/sysy-review-20260914-build -j2
autotest -riscv -s lv1 -w /tmp/sysy-review-20260914-lv1 .
autotest -koopa -s lv3 -w /tmp/sysy-review-20260914-lv3 .
autotest -koopa -t /tmp/sysy-review-20260914-cases -w /tmp/sysy-review-20260914-custom .
```

## 下一步学习建议

先跟踪 `return 1+2*3;` 从 AST 到 raw Koopa 的运算结果引用，再补 Lv3.1/Lv3.2 的 RISC-V binary 与返回值处理；随后实现 Lv3.3，并运行完整 Lv3 的两种模式测试。本次未提前实现后续章节。

对照资料：[Lv3.1 一元表达式](https://pku-minic.github.io/online-doc/#/lv3-expr/unary-exprs)、[Lv3.2 算术表达式](https://pku-minic.github.io/online-doc/#/lv3-expr/arithmetic-exprs)、[Lv3.4 测试](https://pku-minic.github.io/online-doc/#/lv3-expr/testing)。本次读取了以上章节对应 Markdown 原文。
