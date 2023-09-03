croc
======

the beginnings of a compiler because why not

**very** educational project, pls dont use this for anything production

## testing
:warning: Codegen only works on Windows! :warning:
```console
$ git clone https://github.com/croclang/compiler
$ cd compiler
$ cmake -S . -B build/
$ cd build && make
$ ./croc ../example.croc
$ as code.S -o code.o && ld code.o -o code
$ ./code
```