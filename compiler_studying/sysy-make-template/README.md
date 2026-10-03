# SysY → Koopa IR / RISC-V 学习编译器

基于北京大学 [SysY 编译实践教程](https://pku-minic.github.io/online-doc/#/)和 [Makefile 模板](https://github.com/pku-minic/sysy-make-template)完成，使用 C++17、Flex、Bison 和 libkoopa。

已实现整数表达式、常量与变量、作用域、条件与短路、循环及跳转、函数定义与调用、全局变量、SysY 库函数，以及一维／多维数组和数组参数。源码经 AST 生成 Koopa 文本，再通过 libkoopa 解析并生成 RISC-V 汇编。

理解各阶段在代码中的位置，可先读[前端、中端、后端与当前代码的对应关系](docs/compiler-pipeline.md)。

Lab9 最终功能验证在两种模式下均通过完整 Lv9 22/22、Lv8 回归 12/12 和数组参数专项 4/4。仓库整理后的复现结果见[测试报告](docs/test-report.md)。

Lv9+ 的寄存器缓存、常量下标地址优化及性能测量见[实践复盘](docs/labs/Lab9+-寄存器缓存与性能测试复盘.md)。

## 环境与构建

使用[课程开发环境](https://pku-minic.github.io/online-doc/#/misc-app-ref/environment)，其中提供 `clang++`、`flex`、`bison`、libkoopa、`koopac`、RISC-V 工具链和 `autotest`。测试脚本需要 Python 3。

在本目录执行：

```bash
make -j2
```

编译器生成于 `build/compiler`。Makefile 使用课程环境的 `CDE_INCLUDE_PATH` 与 `CDE_LIBRARY_PATH`，也支持课程评测传入的 `BUILD_DIR`、`INC_DIR` 和 `LIB_DIR`。不要用清空链接参数的方法绕过 libkoopa。

## 编译一个程序

```bash
./build/compiler -koopa examples/return42.c -o build/return42.koopa
./build/compiler -riscv examples/return42.c -o build/return42.s
./build/compiler -perf examples/return42.c -o build/return42-perf.s
```

编译器接受 `模式 输入文件 -o 输出文件`。`-koopa` 输出 IR，`-riscv` 和 `-perf` 输出汇编；`-perf` 供课程性能测试使用。生成汇编后的链接和执行由课程测试工具完成。

## 运行测试

在课程环境中运行：

```bash
# 两种模式：自定义测试 + 课程 Lv8 + 课程 Lv9
python3 scripts/test.py

# 只运行一种模式或一个范围
python3 scripts/test.py --mode koopa --suite custom
python3 scripts/test.py --mode riscv --suite lv9

# 课程性能测试
autotest -perf -s perf .
```

`--mode` 可选 `both`、`koopa`、`riscv`；`--suite` 可选 `all`、`custom`、`lv8`、`lv9`。脚本从自身位置定位项目，可以从其他工作目录调用。

每轮由 `autotest` 在临时目录重新构建源码。脚本同时检查进程退出码和实际 `PASSED (n/n)` 汇总，因为当前课程脚本在部分测试失败时也可能返回 0。失败、缺少汇总或没有用例都会使本脚本返回非零状态。

日志和 JSON 汇总保存在 `build/test-logs/<时间>/`，可用 `--log-dir` 指定日志父目录。结项材料保存在 [docs/validation](docs/validation/README.md)。

课程测试继续使用开发环境中的 `/opt/bin/testcases`；本仓库收录 19 个自定义用例，详见 [tests/README.md](tests/README.md)。

## 文件位置

```text
src/                 Flex/Bison 规则、入口和 RISC-V 后端
AST/                 AST、符号表与 Koopa 生成逻辑
docs/labs/           各实验阶段的复盘
docs/test-report.md  结项和归档验证结果
docs/validation/     已归档的日志、汇总和输出示例
tests/array-init/    15 个初始化与数组访问回归用例
tests/array-param/   4 个数组传参用例
examples/           最小 SysY 输入和早期学习示例
scripts/test.py      复现测试入口
build/              本地构建产物与临时测试日志，不提交
```

早期练习位于 `examples/learning/`；其中 `hello.cpp` 是宿主机 C++ 练习，不作为 SysY 输入。阅读顺序见[复盘目录](docs/README.md)。

## 当前边界

这是以课程合法输入和实验测试为范围的学习实现。常量数组元素的编译期求值、完整语义错误恢复及运行时数组越界检查尚未实现；当前已有基本块内的寄存器缓存与常量数组下标化简，全局寄存器分配和进一步性能优化属于后续学习内容。

项目沿用课程 Makefile 模板。Git 仓库根目录在上两级 `PKU_complier/`，编译器目录不再包含独立 `.git`。
