# calcparser

A simple example for compiler, run-time parsing and virtual machine.

### calcparser.py
Generate calculator runtime file (\*.calc) by specified input. Grammars of input is like c,
but with only one varible type. Declarison starts by 'set'.

Usage:

    python calcparser.py [inputname]

### calculator
Just a calculator. It acquires direct input, like "1+1", or a more compilcated one, like a file.

Usage:

    ./calculator
    ./calculator -i [inputname]
    ./calculator < input

