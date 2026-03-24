# LiveLy Programming Language

LiveLy is a custom statically-typed procedural programming language 
with its own compiler, virtual machine, and JIT execution engine.

## Features
- Custom language design
- Compiler pipeline (Lexer → Parser → Semantic → Bytecode)
- Stack-based Virtual Machine
- JIT compilation (planned)

## Example
bind x:int is 10;
emit x;

## Tooling Requirements
- CMake: 3.25+
- Ninja: 1.11+
- C++ compiler with C++17 support (GCC 11+, Clang 14+, or MSVC 19.3+)
- VS Code extension (recommended): CMake Tools (ms-vscode.cmake-tools), latest stable

## Single Command Build + Test
From project root, run:

```powershell
cmake --workflow --preset ci
```

This command performs configure + build + test in one step.

As new tests are added for lexer, parser, AST, semantic analysis, VM, etc. via CTest,
the same command continues to run all tests automatically.

## VS Code Setup
Project settings enforce CMake Presets mode.
Open the folder in VS Code and run the default workflow or test from CMake Tools.