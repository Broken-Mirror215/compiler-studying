# PKU SysY 编译器学习项目

跟随北京大学编译实践教程完成的 C++17 编译器，支持从 SysY 源码生成 Koopa IR 和 RISC-V 汇编。Lv1～Lv9 功能学习已完成，结项验证见[测试报告](compiler_studying/sysy-make-template/docs/test-report.md)。

实际编译器位于 [compiler_studying/sysy-make-template](compiler_studying/sysy-make-template/README.md)。运行 `make`、`autotest` 时使用该目录。

- [构建、运行与测试](compiler_studying/sysy-make-template/README.md)
- [实验复盘目录](compiler_studying/sysy-make-template/docs/README.md)
- [自定义回归测试](compiler_studying/sysy-make-template/tests/README.md)
- [最终验证材料](compiler_studying/sysy-make-template/docs/validation/README.md)

本仓库根目录是唯一 Git 仓库。早期 `T1` 副本已退出版本管理，当前版本以这里的源码、测试和复盘为准。

## 在现有开发容器中复现

本机使用 `sysy-dev` 容器，仓库挂载在 `/workspace`：

```bash
docker exec -w /workspace/compiler_studying/sysy-make-template sysy-dev make -j2
docker exec sysy-dev python3 /workspace/compiler_studying/sysy-make-template/scripts/test.py
```

新的开发环境请按[课程环境说明](https://pku-minic.github.io/online-doc/#/misc-app-ref/environment)准备工具，再使用编译器目录中的命令。
