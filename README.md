# EPQ

This is a programming language that I have developed for my EPQ. It is designed to be language-agnostic and imperative with clean and intuitive symbolic syntax. Various code examples, including various sorting algorithms, are present in the Examples folder.

## Build Instructions

```bash
g++ main.cpp compiler.cc driver.cc exptree.cc lex.yy.c parser.tab.cc program.cc value.cc vm.cc -std=c++17 -O3 -flto -march=native -DNDEBUG

./a.exe FILENAME
```

Use 
```bash
./a.exe -h
```

for execution instructions.

## Advanced Build Instructions

To build the ```parser.tab.cc``` and ```lex.yy.cc``` files from the lexer and parser, use 
```bash
flex scanner.ll
bison -d parser.yy
```