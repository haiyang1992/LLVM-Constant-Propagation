# catpass

This is the template to use for assignments of the Code Analysis and Transformation class at Northwestern

To build:
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

To run:
    $ clang -Xclang -load -Xclang build/catpass/libCatPass.* program_to_analyse.c
