# 自定义回归测试

这里保存原先位于容器 `/tmp` 的自定义测试，共 19 个。课程原始测试不重复复制，继续由开发环境的 `autotest -s lv8`、`autotest -s lv9` 提供。

| 目录 | 数量 | 来源与范围 |
| --- | --- | --- |
| `array-init/` | 15 | 原 `/tmp/sysy-lv92-final-tests` 中的自定义用例，覆盖一维和多维数组、常量与变量、全局与局部、嵌套初始化、补零、运行时求值顺序、短路和输入 |
| `array-param/` | 4 | 原 `/tmp/sysy-lv93-step3-tests`，覆盖参数转传、子数组、常量数组只读传参、修改原数组、第九个指针参数 |

每个 `.c` 都有同名 `.out`，需要标准输入时另有 `.in`。按课程格式，`.out` 包含程序标准输出，最后一行是退出码；没有标准输出时只有退出码。

在课程环境中，从编译器项目目录运行：

```bash
python3 scripts/test.py --suite custom
```

也可直接使用课程工具运行某个目录：

```bash
autotest -koopa -t tests/array-init .
autotest -riscv -t tests/array-param .
```

用例名称、期望输出和输入文件按原材料保留。Lab9.2 的历史 26 个用例由 11 个课程原始用例和这里的 15 个自定义用例组成；Lab9.3 专项 4 个全部保存在 `array-param/`。
