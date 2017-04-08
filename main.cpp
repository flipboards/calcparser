#include <iostream>
#include "calc.h"
#include "error.h"
#include "string.h"

using namespace std;
using namespace calc;

void display_help() {
	printf("Usage: calc {-i [infile]} {-r} {-h}\n");
}

void help_function() {
	printf("Type list to get all functions.\n");
	printf("Type help <function> to get function signature.\n");
}

const char* parse_error(int err) {
	switch (err)
	{
	case ERROR_BAD_VAR: return "Runtime error %d: Bad variable name\n";
	case ERROR_BAD_FUNC: return "Runtime error %d: Bad function name\n";
	case ERROR_STACK_SYM: return "Runtime error %d: Symbol stack error\n";
	case ERROR_STACK_CALL: return "Runtime error %d: Call stack error\n";
	case ERROR_STACK_FRAME: return "Runtime error %d: Frame stack error\n";
	case ERROR_STACK_OVERFLOW: return "Runtime error %d: Stack overflow\n";
	case ERROR_JUMP: return "Runtime error %d: Error jump address\n";
	case ERROR_NO_MAIN: return "Runtime error %d: Couldn't locate main function\n";
	default:
		return "Runtime error %d\n";
	}
}

int main(int argc, char** argv) {
	string input;
	Calculator calculator;
	Number result;
	enum RunMode
	{
		MODE_INPUT, MODE_RUN
	};
	RunMode mode = MODE_INPUT;
	FILE* file = nullptr;

	if (argc > 1) {

		// file input
		if (strcmp(argv[1], "-i") == 0) {
			if (argc == 3) {
				if (!(file = freopen(argv[2], "r", stdin))) {
					printf("Invalid file input.\n");
					return 1;
				}
				mode = MODE_RUN;
			}
			else {
				display_help();
				return 0;
			}
		}

		else if (strcmp(argv[1], "-h") == 0) {
			display_help();
			return 0;
		}

		else if (strcmp(argv[1], "-r") == 0) {
			mode = MODE_RUN;
		}
		else {
			display_help();
			return 0;
		}
	}
	
	int linecnt = 0;
	if(mode == MODE_INPUT) printf(">>");
	while (getline(cin, input)) {

		// input mode
		if (mode == MODE_INPUT) {
			if (input == "exit") {
				break;
			}
			else if (input == "run") {
				calculator.exec();
			}
			else if (input == "help") {
				help_function();
			}
			else if (input == "list") {
				for (const auto& f : calculator.get_all_functions())printf("%s ", f.c_str());
				printf("\n");
			}
			else if (input.substr(0, 5) == "help ") {
				int fargc;
				bool ftype, freturns;
				if (!calculator.get_function_signature(input.substr(5, input.size()), fargc, ftype, freturns)) {
					const char* description = ftype ? "Defined" : "Built-in";
					printf("%s function %s: Accept %d args, return %d args.\n", description, &input[5], fargc, freturns ? 1 : 0);
				}
				else {
					printf("Function %s does not exist.\n", &input[5]);
				}
			}
			else if(input.substr(0, 3) == "set"){
				if (calculator.parse_unchecked(input)) {
					printf("Invalid input!\n");
				}
			}
			else {
				calculator.parse_unchecked("setf main:0,0,1 &r=print(" + input + ";)");
				if (calculator.exec() > 0) {
					printf(parse_error(calculator.get_error()), calculator.get_error());
				}
			}
			printf(">>");
		}

		// run mode
		else if (mode == MODE_RUN) {
			if (calculator.parse_unchecked(input)) {
				printf("Invalid input for line %d.\n", linecnt);
				linecnt = -1;
				break;
			}
		}
		linecnt++;
	}
	
	if (mode == MODE_RUN && linecnt > 0) {
		if (calculator.exec() > 0) {
			printf("Runtime Error %d\n", calculator.get_error());
		}
	}
	if (file) {
		fclose(file);
	}

	return 0;
}