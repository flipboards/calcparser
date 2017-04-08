#include "builtin.h"

Number _self(Number* num, int*) {
	return *num;
}
Number _self2(Number* num, int*) {
	return num[1];
}
Number _shift(Number* num, int*) {
	Number tmp = num[0];
	*(int*)&tmp += (int)num[1].real() * sizeof(Number)/sizeof(MYBIT);
	return tmp;
}

Number _plus(Number* num, int*) {
	return num[0] + num[1];
}
Number _minus(Number* num, int*) {
	return num[0] - num[1];
}
Number _mul(Number* num, int*) {
	return num[0] * num[1];
}
Number _div(Number* num, int*) {
	return num[0] / num[1];
}
Number _pow(Number* num, int*) {
	return std::pow(num[0], num[1]);
}

Number _greater(Number * num, int *)
{	
	return num[0].real() > num[1].real() ? Number(1.0) : Number(0.0);
}

Number _less(Number * num, int *)
{
	return num[0].real() < num[1].real() ? Number(1.0) : Number(0.0);
}

Number _equal(Number * num, int *)
{
	return num[0].real() == num[1].real() ? Number(1.0) : Number(0.0);
}

Number _sin(Number* num, int*) {
	return sin(*num);
}
Number _cos(Number* num, int*) {
	return cos(*num);
}
Number _tan(Number* num, int*) {
	return tan(*num);
}

Number _sqrt(Number* num, int*) {
	return sqrt(num[0]);
}

Number _exp(Number * num, int *)
{
	return exp(*num);
}
Number _exp10(Number* num, int*) {
	return pow(10.0, num[0]);
}
Number _log(Number* num, int*) {
	return log(num[0]);
}
Number _log10(Number* num, int*) {
	return log10(num[0]);
}

#define PRINT_CUTOFF 1e-10

Number _print(Number* num, int*) {
	if (abs(num->real()) < PRINT_CUTOFF) {
		if (abs(num->imag()) < PRINT_CUTOFF) {
			printf("0\n");
		}
		else {
			printf("%.3fi\n", num->imag());
		}
	}
	else {
		printf("%.3f", num->real());
		if (abs(num->imag()) > PRINT_CUTOFF) {
			if (num->imag() > 0) {
				printf("+%.3fi\n", num->imag());
			}
			else {
				printf("%.3fi\n", num->imag());
			}
		}
		else {
			printf("\n");
		}
	}
	return *num;
}