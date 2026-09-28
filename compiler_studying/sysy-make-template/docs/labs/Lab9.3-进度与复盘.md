# Lab9.3：数组参数进度与复盘

> 归档说明：本文保留对应实验阶段的实现记录，早期代码结构和行号可能已变化。当前状态见[项目 README](../../README.md)，可复现的用例和验证材料见[测试报告](../test-report.md)。历史 `/tmp` 路径不再作为复现输入。

更新日期：2026-09-28。Lab9.3 数组参数的前端和 RISC-V 后端均已接通，Lv9.4 完整测试已完成。最终当前源码在 Koopa、RISC-V 两种模式下均通过数组参数专项 4/4、完整 Lv9 22/22、完整 Lv8 回归 12/12。本文记录最终实现、代码位置、关键设计和实际验证结果。

参考：[课程 Lv9.3 数组参数](https://pku-minic.github.io/online-doc/#/lv9-array/array-param)、[Lv9.4 测试](https://pku-minic.github.io/online-doc/#/lv9-array/testing)。代码行号对应本次检查，后续编辑会使行号移动。

## 1. 当前进度

| 步骤 | 内容 | 状态 |
| --- | --- | --- |
| 形参表示与函数入口 | 解析数组形参，保存后续维度，输出指针类型，建立参数局部存储和符号信息 | 已完成 |
| 数组参数元素读写 | 取出参数指针，第一个下标用 `getptr`，后续用 `getelemptr` | 已完成 |
| 数组和子数组传递 | 支持 `f(a)`、`f(a[i])` 和数组参数继续转传 | 已完成 |
| RISC-V 后端 | 支持 `getptr` 及指针取值、存储和地址加载 | 已完成 |
| 完整验证 | 数组库函数、完整 Lv9 和 Lv8 回归 | 两种模式均通过 |

## 2. 已完成的形参结构与语法

| 实现内容 | 当前代码位置 | 作用 |
| --- | --- | --- |
| 数组参数符号种类 | `AST/BaseAst.h:35` | `ArrayParam` 区分数组本体和保存数组地址的参数 |
| 单个形参 AST | `AST/BaseAst.h:175` | 保存 `ident`、`is_array` 和后续维度表达式 `array_dims` |
| 函数形参列表 | `AST/BaseAst.h:226`、`:297` | 通过 `unique_ptr<FuncFParamAst>` 保存完整节点 |
| 形参语法 | `src/sysy.y:167` | 解析 `INT IDENT` 和 `INT IDENT '[' ']' ArrayDims` |
| 收集形参列表 | `src/sysy.y:184` | 将形参节点所有权交给列表，随后移动给函数定义 |
| 收集后续维度 | `src/sysy.y:552` | 复用 9.2 的 `ArrayDims`，允许没有后续维度 |
| 生成参数类型 | `AST/BaseAst.h:708` | 普通参数输出 `i32`；数组参数复用维度求值和数组类型构造，再加 `*` |
| 输出函数入口 | `AST/BaseAst.h:455` | 输出参数类型，分配局部存储位置，保存传入值并登记符号 |

第一对空 `[]` 用 `is_array` 表示，不保存为长度表达式。例如：

| 源语言形参 | 后续维度 | Koopa 参数类型 | 源语言维数 |
| --- | --- | --- | --- |
| `int x` | 无 | `i32` | 0 |
| `int a[]` | 无 | `*i32` | 1 |
| `int a[][3]` | `{3}` | `*[i32, 3]` | 2 |
| `int a[][2][3]` | `{2, 3}` | `*[[i32, 3], 2]` | 3 |

`int x` 和 `int a[]` 的维度列表都为空，因此不能只用 `array_dims.empty()` 判断是否为数组参数。数组参数的 `array_rank` 为 `array_dims.size() + 1`。

后续维度使用 `CalcArrayDims()` 在编译期求值，支持已定义的标量常量表达式。`ArrayType()` 继续复用 9.2 从内向外构造嵌套类型的逻辑。

## 3. 参数存储位置与数组地址

数组参数传递地址。函数入口仍为形参建立局部存储位置，但其中保存的是指针，而不是数组本体。

当前已验证的输入之一：

```c
int one(int a[]) {
  return 0;
}
int main() {
  return 0;
}
```

对应入口结构：

```koopa
fun @one(@a: *i32): i32 {
%entry:
  %var_a_0 = alloc *i32
  store @a, %var_a_0
  ret 0
}
```

`@a` 是 `*i32`，`%var_a_0` 是 `**i32`。在课程的 RV32 环境中，`alloc *i32` 为一个指针分配 4 字节空间。

对于 `int a[][3]`，传入值是 `*[i32, 3]`，局部存储位置则是 `**[i32, 3]`。

## 4. 已接通的完整元素访问

代码位于 `AST/BaseAst.h:721` 的 `LValAst`，赋值节点位于 `AST/BaseAst.h:1190`。

完整元素访问的设计为：

```text
普通数组：数组本体地址 → 逐维 getelemptr → 元素地址
数组参数：参数存储位置 → load 指针 → 第一次 getptr
          → 后续 getelemptr → 元素地址
```

`Address()` 计算元素地址；`Dump()` 再 `load` 得到整数；`AssignStmtAst` 调用 `Address(true)`，随后 `store` 写入整数。

以 `int a[][3]` 中的 `a[1][2]` 为例，实际生成的 IR 已通过 Koopa 解析。下列按含义重新命名临时值，展示其访问结构：

```koopa
%base = load %slot
%row = getptr %base, 1
%elem = getelemptr %row, 2
%value = load %elem
```

| 值 | 类型 | 含义 |
| --- | --- | --- |
| `%slot` | `**[i32, 3]` | 保存参数指针的局部存储位置 |
| `%base` | `*[i32, 3]` | 传入的行地址 |
| `%row` | `*[i32, 3]` | 第二行地址，前进 12 字节 |
| `%elem` | `*i32` | 第二行第三个整数地址，再前进 8 字节 |
| `%value` | `i32` | 读取的整数 |

`getptr` 保持指针类型，按其指向对象的大小移动；`getelemptr` 从数组中取元素地址，指针类型随之变为指向元素的类型。

当前读取允许下标数量小于或等于数组维数，赋值仍要求下标写全。超过维数的访问、给整个数组或子数组赋值，以及直接修改常量数组均会被拒绝。

### 数组与子数组传递

`LValAst::Dump()` 根据下标数量决定生成地址还是读取整数：

| 情况 | 生成方式 |
| --- | --- |
| 标量常量 | 返回编译期整数 |
| 标量变量或完整数组元素 | 计算地址后 `load` 整数 |
| 普通数组名或剩余子数组 | 计算地址后 `getelemptr ..., 0`，取得首元素地址 |
| 没有下标的数组参数 | 从参数存储位置 `load` 出指针，直接作为结果 |

例如普通数组 `int b[2][3]` 的本体地址类型为 `*[[i32, 3], 2]`，经过 `getelemptr b, 0` 后得到 `*[i32, 3]`，与 `int a[][3]` 的形参类型一致。地址数值不变，但指针所指向的类型由整个数组变成第一行。

数组参数本身已经具有上述指针类型，直接转传时不再进行这一步。部分下标访问数组参数时，先用 `getptr` 和后续 `getelemptr` 定位剩余子数组，再通过 `getelemptr ..., 0` 取得其首元素地址。

现有 `FuncCallAst::Dump()` 调用每个实参的 `Dump()`，收集 `result` 并输出 `call`，因此能够复用这条路径传递指针结果。Koopa 解析负责检查最终生成的调用类型是否一致；当前未增加独立的前端实参类型系统。

## 5. RISC-V 后端怎样处理指针

| 实现内容 | 当前代码位置 | 作用 |
| --- | --- | --- |
| 类型大小 | `src/main.cpp:27` | RV32 的整数及指针大小均为 4 字节，数组类型递归计算大小 |
| 指针地址加载 | `src/main.cpp:42` | 全局对象用 `la`，局部对象计算 `sp + offset`；`load`、`getptr`、`getelemptr` 的指针结果从结果栈槽取出 |
| 栈帧分配 | `src/main.cpp:69` | 局部对象按对象类型分配，普通指令结果包括指针结果各占 4 字节 |
| 指针取值 | `src/main.cpp:174` | `LoadValue()` 接受整数或指针结果，用已有的参数寄存器和栈槽逻辑传递值 |
| 数组元素地址 | `src/main.cpp:425` | `getelemptr` 按源指针指向数组的元素类型大小计算偏移 |
| 指针运算 | `src/main.cpp:438` | `getptr` 按源指针指向对象的完整类型大小计算偏移 |

### 指针值和存储位置的地址

局部 `alloc` 的结果代表一块存储位置，因此加载该地址需要计算 `sp + offset`。计算指令的指针结果则作为一个数值保存在栈槽中，使用时要 `lw` 取出其中保存的地址，不能再次将该栈槽的地址当成计算结果。

这也是 `LoadAddress()` 需要同时处理 `KOOPA_RVT_LOAD`、`KOOPA_RVT_GET_PTR`、`KOOPA_RVT_GET_ELEM_PTR` 的原因：这些值在当前前端产生的地址访问链中都是指针结果。

### 两种指针运算的步长

| 指令 | 步长计算 | 源指针为 `*[i32, 3]` 时 |
| --- | --- | --- |
| `getptr` | `TypeSize(src->ty->data.pointer.base)` | 一整行，12 字节 |
| `getelemptr` | `TypeSize(array_ty->data.array.base)` | 一个整数，4 字节 |

`getptr` 的 RISC-V 计算过程为：先取得源指针和下标，将下标乘以指向对象的字节大小，加到源地址上，然后把新指针保存到当前指令的结果栈槽。

### 实际传参片段

专项 `chain` 中的函数：

```c
int row(int a[][3]) {
  return elem(a[1]);
}
```

最终编译器实际输出：

```koopa
fun @row(@a: *[i32, 3]): i32 {
%entry:
  %var_a_1 = alloc *[i32, 3]
  store @a, %var_a_1
  %3 = load %var_a_1
  %4 = getptr %3, 1
  %5 = getelemptr %4, 0
  %6 = call @elem(%5)
  ret %6
}
```

对应 `getptr %3, 1` 的实际汇编片段：

```asm
lw t0, 4(sp)
li t1, 1
li t2, 12
mul t1, t1, t2
add t0, t0, t1
sw t0, 8(sp)
```

随后 `getelemptr %4, 0` 的步长是 4 字节，但下标为 0，不再移动地址；它把行指针转换成首整数指针。该指针从结果栈槽装入 `a0`，然后 `call elem`。

现有的函数调用逻辑和超过八个参数的栈传参逻辑可以复用。指针和整数在当前 RV32 环境中同为 4 字节；专项中的第九个指针参数已通过实际执行验证。

观察文件保存在容器 `/tmp/lv93-final-chain.koopa` 和 `/tmp/lv93-final-chain.s`。

## 6. 最终验证结果

当前源码在 `sysy-dev` 中重新执行 `make -j2`，构建通过。课程 `autotest` 随后在各自临时构建目录重新编译当前源码并执行程序：

| 执行范围 | Koopa | RISC-V |
| --- | --- | --- |
| 数组参数专项 | 4/4 | 4/4 |
| 完整 Lv9 | 22/22 | 22/22 |
| 完整 Lv8 回归 | 12/12 | 12/12 |

六轮最终执行测试均在本次后端改动完成后运行，分别重新构建当前源码。两种模式合计执行 76 次用例，全部通过，没有使用修改前的旧编译器作为最终验证依据。

专项用例目录为容器 `/tmp/sysy-lv93-step3-tests`：

| 用例 | 验证内容 | 返回值 |
| --- | --- | --- |
| `chain` | 整个二维数组传参、参数转传、普通数组和数组参数的子数组传递 | 7 |
| `const_arrays` | 全局及局部常量数组、整个数组及子数组只读传递 | 30 |
| `modify_original` | 经参数转传修改全局及局部原数组 | 18 |
| `ninth_pointer` | 第九个形参为数组指针，与八个整数参数混合 | 45 |

`chain` 的计算为第二行第二个整数 5，加第一行第二个整数 2，得到 7。`modify_original` 通过函数分别把两个数组的 `[1][1]` 改为 9，读取后相加得到 18。

完整 Lv9 包含数组参数、数组库函数和排序程序。这些结果来自实际执行及与期望输出比较，超出了仅检查 IR 能否解析的范围。

此前形参和元素读写步骤还执行过以下最小 Koopa 生成检查：

| 用例 | 验证内容 | 结果 |
| --- | --- | --- |
| `scalar` | 普通整数形参读取及调用 | 通过 |
| `types` | 一至三维数组形参类型，后续维度引用标量常量 | 通过 |
| `read` | 二维数组参数的完整元素读取 | 通过 |
| `write` | 一维数组参数的元素赋值 | 通过 |
| `dynamic3d` | 三维数组参数的动态下标读写，混合整数形参 | 通过 |

上述 5 个输入均成功生成并通过 Koopa 解析。另检查了读取与写入中的指令组合：二维读取包含一次 `getptr` 和一次 `getelemptr`；一维赋值包含一次 `getptr` 和 `store 9`；三维动态读写共包含两次 `getptr`、四次 `getelemptr` 和 `store 42`。

上述 5 个最小检查中的“通过”指成功生成并经过 Koopa 解析的 IR；完整执行结果以本节前面的专项与课程测试表为准。

当前材料位于容器：

- 构建日志：`/tmp/lv93-step2-build.log`。
- 输入：`/tmp/lv93-step2-scalar.c`、`/tmp/lv93-step2-types.c`、`/tmp/lv93-step2-read.c`、`/tmp/lv93-step2-write.c`、`/tmp/lv93-step2-dynamic3d.c`。
- 结果记录：`/tmp/lv93-step2-results.json`。
- 最终构建日志：`/tmp/lv93-final-build.log`。
- 最终 Koopa 日志：`/tmp/lv93-final-custom-koopa.log`、`/tmp/lv93-final-lv9-koopa.log`、`/tmp/lv93-final-lv8-koopa.log`。
- 最终 RISC-V 日志：`/tmp/lv93-final-custom-riscv.log`、`/tmp/lv93-final-lv9-riscv.log`、`/tmp/lv93-final-lv8-riscv.log`。
- 最终汇总：`/tmp/lv93-final-results.json`。

复现示例：

```bash
cd /workspace/compiler_studying/sysy-make-template
make -j2
autotest -koopa -t tests/array-param .
autotest -koopa -s lv9 .
autotest -koopa -s lv8 .
autotest -riscv -t tests/array-param .
autotest -riscv -s lv9 .
autotest -riscv -s lv8 .
```

临时文件可能随容器清理而消失。测试结论及关键输出已记录在本文。

## 7. 完成范围与后续学习起点

Lab9.1 一维数组、Lab9.2 多维数组、Lab9.3 数组参数均已实现，Lab9.4 完整 Lv9 测试在两种输出模式下均通过。本轮没有尚待完成的 Lab9 功能代码改动。

当前的常量数组元素编译期求值、完整错误恢复和运行时越界检查沿用之前的实现边界，不属于本轮新增功能。完成结论对应上述课程范围和实际执行测试。

后续如继续学习，可进入课程 Lv9+ 的寄存器分配、优化及 SSA 等主题；开始前由用户选择学习方向。

继续采用用户手写源码、助手检查并重新构建验证的方式。笔记记录最终设计与实际结果，不记录手写错误及修正过程。
