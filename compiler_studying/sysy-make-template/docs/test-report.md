# 结项验证与复现记录

日期：2026-09-28。项目完成 Lv1～Lv9 的功能学习，支持 SysY → Koopa IR 和 SysY → RISC-V 汇编。本页记录最终功能验证及文件归档后的复现结果。

## 文件归档后的复现

本轮仅整理项目文件、测试、文档和脚本，未改动编译器源码及 Makefile。前后 SHA-256 一致，见 [source-sha256.json](validation/closeout-2026-09-28/source-sha256.json)。

使用仓库内脚本运行六轮测试：

```bash
python3 scripts/test.py
```

| 范围 | Koopa | RISC-V |
| --- | --- | --- |
| 自定义测试：15 个数组初始化与访问 + 4 个数组参数用例 | 19/19 | 19/19 |
| 完整课程 Lv8 | 12/12 | 12/12 |
| 完整课程 Lv9 | 22/22 | 22/22 |
| 合计 | 53/53 | 53/53 |

两种模式共执行 106 次用例，全部通过。每一轮由课程 `autotest` 在独立临时构建目录重新编译当前源码，随后执行生成的程序，并比较实际输出与期望输出。

结果及原始日志均已归档：

- [JSON 汇总](validation/closeout-2026-09-28/results.json)
- Koopa：[自定义](validation/closeout-2026-09-28/koopa-custom.txt)、[Lv8](validation/closeout-2026-09-28/koopa-lv8.txt)、[Lv9](validation/closeout-2026-09-28/koopa-lv9.txt)
- RISC-V：[自定义](validation/closeout-2026-09-28/riscv-custom.txt)、[Lv8](validation/closeout-2026-09-28/riscv-lv8.txt)、[Lv9](validation/closeout-2026-09-28/riscv-lv9.txt)

测试脚本还经过一次独立失败路径检查：模拟课程工具退出码为 0、输出为 `WRONG ANSWER (0/1)`，新脚本正确返回 1。此检查只验证脚本的失败识别，不计入编译器用例数。

## 原 Lab9 功能结项记录

完成 Lab9.3 后，Koopa 和 RISC-V 分别通过数组参数专项 4/4、完整 Lv9 22/22、Lv8 回归 12/12。对应的六轮结果保存在 [Lab9 原始汇总](validation/lab9-2026-09-28/results.json)，共 76 次执行。

这里的专项 4 个用例是本次自定义 19 个用例的子集，不将两个阶段的重复执行数当成新增覆盖数。

## 覆盖内容

- 一维和多维数组、全局和局部数组、变量及常量数组。
- 嵌套及混合初始化列表、补零、空列表、运行时初值顺序与短路。
- 整个数组和子数组传参、参数转传、经参数修改原数组。
- 指针参数与整数参数混合，包括第九个参数通过栈传递。
- 课程数组库函数、排序程序，以及 Lv8 的函数调用和全局变量回归。

当前源码边界与学习说明见 [README](../README.md)。测试通过的结论对应这些课程用例及已归档自定义用例。

## 后续复现与保存结果

在课程开发环境中从项目目录运行 `python3 scripts/test.py`，或者选择单一模式／范围：

```bash
python3 scripts/test.py --mode riscv --suite lv9
python3 scripts/test.py --mode koopa --suite custom
```

脚本自动定位项目，每次日志保存在 `build/test-logs/<时间>/`。需要归档新的结果时，将对应日志和 JSON 汇总放入一个新的 `docs/validation/` 子目录，并记录测试时的 Git 提交。

预期输出 `.out` 是测试输入资料，应随 Git 保存；编译得到的可执行文件、对象文件以及临时日志放在被忽略的 `build/` 中。
