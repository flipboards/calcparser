## CalcParser User Manual

version: 0.0-alpha

## Instructions

### Operator
There are 5 operator currently, all of which can be overloaded.

Priority: ^ = 3; \*, / = 2; +, - =1; Brakets (including simple braket or functions) = 0;

### Comma
Either semicolon (";") or line-change will regarded as the end of sentence. Multiple spaces are regarded as one,
while they will be neglected near a comma ",", braket "(", or operator "+", "-", "\*", "/", "^".

For instance:

	>>1 + 2
	3
	>> atan(1  , 2)
	0.707
	>> a=2;a+2;
	4

### Variables
**Global variables** can be declared by 'set' outsider a function. There must be a constant argument.

	set a 2;
Expression is unsupported for global variables. Below are pre-defined global variables:

	pi = 3.1415926
	e = 2.7182818285
	i = 0.0 + 1.0i

Once a global variable is set, it cannot be changed inside any function. But can be deleted by 'del':

	del a;

**Local variables** can also be declared by 'set', with no arguments.

	set b;
	b = 2;


### Functions

#### In-line functions

One can set a function by 'set':

	set foo(a, b){a=a+b;return a;}

Functions cannot be defined inside a function.

Functions cannot be overloaded. Redefine a function will overlap its previous definition.

In line input mode, a 'main' function with no argument will be automaticly built. But in input file
it is required explicitly. The return of main will be print out.

#### Invoke functions

CalcParser also supports c/c++ invoke functions by calling **set_operator**. In fact, built-in functions
are all set by set_operator.

	set_operator(const std::string& name, const Operator&);

An operator can be created by 

	Operator(int priority, int argc, (Number*)func(Number* argv, int* err))

### Byte Codes
Byte codes are used as direct input, which has more strict rules. Parser will not check grammar, which means
grammar problems would probabily cause the program to crash.

#### Instructions
All instructions should write within one line. No spaces are allowed. Instruction **setf** and **setv** are used rather than **set**.

#### Functions
Functions begin with **setf**. After a space, name is seperated by ":" with three numbers, meaning
argument number, stack size and whether it returns (0/1). For example:
    
    setf p:1,0,0 print($a1);
    setf main:0,0,1 &r=print(2);

The main function will be the entry for running.

#### Variables
The first char of variables begin with rather "$", "&" or "@", meaning direct use, reference (pointer) or dereference.
Assignment ("=") to a number should begin with its pointer. For example:

    >>setf main:0,1,1 &l0=2;print($l0);
    >>run
    2.000
    
The second char of variables means the *place* of the variable.
- a: Argument variable, lower than frame pointer;
- l: Local variable, higher than frame pointer;
- r: Shared register;

Next is the offset which is always positive or zero. Offset for argument pointer begins from first argument (bottom);
Offset for local variable pointer begins from frame pointer.

#### Builtin
The following functions are built in CalcParser:
- Arithmetic operators: '+','-','\*','/','^': Mathmatical meaning;
- Comparasion: '>', '<', 'eq()': Return 0(float) if false, 1(float) if true;
- Heap allocation: **new**(a) to allocate an array of length a.

        >>setf main:0,1,0 &l0=new(5);$l0=2;print(@l0);
        >>run
        2.000

- Pointer calculation: **index**(p, shift) to add shift to p. Shift can be positive or negative.
- Print results: **print**(a) to print a result. It returns the value of a.