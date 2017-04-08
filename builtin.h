#pragma once

#include "calc.h"

// functional

// return *num
Number _self(Number* num, int*);
// return *(num+1)
Number _self2(Number* num, int*);

// shift pointer (int)
Number _shift(Number* num, int*);

// math operator

Number _plus(Number* num, int*);
Number _minus(Number* num, int*);
Number _mul(Number* num, int*);
Number _div(Number* num, int*);
Number _pow(Number* num, int*);

// comparison

Number _greater(Number* num, int*);
Number _less(Number* num, int*);
Number _equal(Number* num, int*);

// math extended

Number _sqrt(Number*, int*);
Number _sin(Number* num, int*);
Number _cos(Number* num, int*);
Number _tan(Number* num, int*);
Number _exp(Number* num, int*);
Number _exp10(Number*, int*);
Number _log(Number*, int*);
Number _log10(Number*, int*);

// extended

Number _print(Number* num, int*);
