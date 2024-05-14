# SysY-Compiler

SysY是一个C语言的子集

## 使用方法

首先 clone 本仓库, 进入仓库目录后执行:

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
```

运行命令形如:

```sh
build/compiler -koopa hello.c -o hello.koopa
build/compiler -riscv hello.c -o hello.riscv
```

## 标准运行环境

若需要本地运行参考[compiler-dev](https://github.com/pku-minic/compiler-dev)

