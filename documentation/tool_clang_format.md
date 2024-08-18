# Clang-Format

All C++ Files are formatted with this

**Make sure you use verson 18!**

## Installation

### Windows

Download it from the clang github website:

https://github.com/llvm/llvm-project/releases

Make sure you install the right version. The correct package is:

[clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz](https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz)

Install it under `C:\Program Files\llvm` and add it to `%PATH%`.

### Linux

Install via the package manager

```bash
sudo apt install clang-format-18
```

## Manual Usage

To manually format all scripts, use this file:

```bash
python3 scripts/tool_clang_format_all.py
```

## Format on Save

It is best to enable *format-on-save* in your IDE.

Make sure you use the same version as in the Python script!


