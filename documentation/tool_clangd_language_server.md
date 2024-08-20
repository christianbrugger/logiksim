# Clang Language Server



To use the Clang language server (clangd) copy the debug config you are using from `.clangd.in` to `.clangd`.



Example `.clangd` when using the `linux-gcc-debug` cmake preset:

```
CompileFlags:
  CompilationDatabase: build/linux-gcc-debug
```



