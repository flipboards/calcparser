#include <math.h>
#include <regex>
#include "calc.h"
//#include "parser.h"
#include "error.h"
#include "builtin.h"
#include "stringutil.h"

using namespace std;
using namespace calc;

#define IS_INT(x)			((x) - 48 >= 0 && (x) - 48 < 10 || x == '.')
#define BRACKET_PRIORITY	0
#define NUMBER_SIZE			sizeof(Number)/sizeof(MYBIT)
#define MAX_STACK			0xffff
#define HEAP_SHIFT			0x40000000

Operator::Operator(int Flag, int Argc, Number(*f)(Number*, int*)) :
	_priority(Flag), argc(Argc), varc(0), func(f), is_function(true), is_string(false){

}

Operator::Operator(int Flag, int Argc, unsigned int Varc, bool Returns, const string& Instruction) :
	_priority(Flag), argc(Argc), varc(Varc), is_function(Returns), instruction(Instruction), is_string(true)
{
}

Operator::Operator() : is_string(false), func(nullptr) {

}

inline int Operator::priority()const {
	return _priority;
}

Calculator::Calculator() : Calculator("x")
{
}
Calculator::Calculator(const string& LastAnswer) : last_answer(LastAnswer), _err(0) {
	_init_operator();
	_init_constant();
	stack_frame.reserve(256);
	variables[last_answer] = Number(0);
}

int Calculator::_run(Number* result) {
	
	string tmp;
	stack<Operator*> stack_sym;
	stack_sym.push(_oper_self);
	int err = 0;

	for (auto pc = stack_call.top()->instruction.begin(); pc != stack_call.top()->instruction.end();) {

		if (*pc== '(') {
			// simple left braket
			if (tmp.empty()) {
				stack_sym.push(_oper_self);
			}
			// function
			else {
				auto sym = operators.find(tmp);
				if (sym == operators.end()) {
					return ERROR_BAD_FUNC;
				}
				else {
					stack_sym.push(sym->second);
				}
				tmp.clear();
			}
		}
		else if (*pc== ')' || *pc== ';'|| *pc == ':') {

			if (!tmp.empty()) {
				if (err = _parse_num(tmp, tmp_var))break;
				_push(tmp_var);
				tmp.clear();
			}

			while (stack_sym.top()->priority() > BRACKET_PRIORITY) {
				if (err = _call(stack_sym.top()))break;
				stack_sym.pop();
			}

			if (err = _call(stack_sym.top()))break;	// left braket.
			stack_sym.pop();
		
			// line changing
			if (*pc== ';' || *pc == ':') {
				if (!stack_sym.empty()) {
					_clear_stack(stack_sym);
				}
				stack_sym.push(_oper_self);
			}

			// jump
			if (*pc == ':') {
				_pop(tmp_var);
				int index = (int)tmp_var.real();
				if (index > (int)stack_call.top()->instruction.size()) {
					return ERROR_JUMP;
				}
				else if(index < 0){}
				else {
					pc = stack_call.top()->instruction.begin() + index;
					continue;
				}
			}
		}

		// single operator
		else {
			auto sym = symple_operators.find(*pc);
			if (sym == symple_operators.end()) {
				tmp.push_back(*pc);
			}
			else {
				if (!tmp.empty()) {
					if (err = _parse_num(tmp, tmp_var))break;
					_push(tmp_var);
					tmp.clear();
				}

				while (sym->second->priority() <= stack_sym.top()->priority()) {
					if (err = _call(stack_sym.top()))break;
					stack_sym.pop();
				}

				stack_sym.push(sym->second);
			}
		}
		pc++;
	}
	return err;
}
int calc::Calculator::_parse_num(const string& name, Number& value)
{
	if (!IS_INT(name[0])) {
		int offset = atoi(&name[2]);
		if (name[0] == '$') {
			value = *_deref(_parse_ptr(name[1], offset));
		}
		else if (name[0] == '&') {			// will return a pointer rather than value
			*(int*)&value = _parse_ptr(name[1], offset);
		}
		else if (name[0] == '@') {
			int* ref_tmp;
			ref_tmp = (int*)_deref(_parse_ptr(name[1], offset));
			if (_err)return _err;
			value = *_deref(*ref_tmp);
		}
		else {
			return get_variable(name, value);
		}
	}
	else {
		value = Number(atof(name.c_str()));
	}
	return _err;
}
int calc::Calculator::_parse_ptr(char option, int offset)
{
	switch (option)
	{
	case 'a':
		return _frame_pointer - 1 - offset * NUMBER_SIZE;
	case 'l':
		return _frame_pointer + offset * NUMBER_SIZE;
	case 'r':
		return -1;
	default:
		_err = ERROR_BAD_VAR;
		return 0;
	}
}
Number* calc::Calculator::_deref(int ptr)
{
	if (ptr < 0) {
		return &shared_result;
	}
	else if (ptr < 0xffff) {
		return (Number*)&stack_frame[ptr];
	}
	else if (ptr < HEAP_SHIFT) {
		_err = ERROR_BAD_VAR;
	}
	else {
		return (Number*)&memory_heap[ptr - HEAP_SHIFT];
	}
	return &shared_result;
}
int calc::Calculator::_call(Operator* function)
{
	if (stack_frame.size() < function->argc) {
		return ERROR_STACK_FRAME;
	}

	if (function == _oper_self)return 0;	// do nothing

	stack_call.push(function);
	_pushl(_frame_pointer);					// save frame pointer
	_frame_pointer = (MYBIT)stack_frame.size();	// stack pointer -> frame pointer

	int result = 0;
	if (function->is_string) {
		stack_frame.resize(stack_frame.size() + function->varc * NUMBER_SIZE);	// prepare stack
		result = _run(&shared_result);
	}
	else {
		Number* argv = (Number*)&stack_frame.back() - function->argc;	// set argument pointer

		// assignment
		if (function == _oper_assign && *(int*)argv >= 0) {				// return variable: index < 0
			*_deref(*(int*)argv) = shared_result = argv[1];
		}
		else if (function == _oper_new) {
			*(int*)&shared_result = (int)memory_heap.size() + HEAP_SHIFT;
			memory_heap.resize(memory_heap.size() + NUMBER_SIZE * (int)argv[0].real());
		}
		else {
			shared_result = function->func(argv, &result);
		}
	}

	stack_frame.resize(_frame_pointer);		// ret
	_popl(_frame_pointer);					// restore frame pointer
	
	stack_frame.resize(stack_frame.size() - function->argc * NUMBER_SIZE);

	if (function->is_function) {		// consider a procedure rather than function
		_push(shared_result);
	}

	stack_call.pop();	// restore pc
	return result;
	
}
/*
int calc::Calculator::parse(const string & input, Number& result)
{
	_err = 0;
	Parser parser;

	switch (parser.parse_line(input)) {
	case 2: 
		variables[parser.variables[0].name] = Number(stod(parser.variables[0].content));
		result = variables[parser.variables[0].name];
		break;
	case 1:
		_set_function(parser.functions[0].name, 
			parser.functions[0].content, 
			parser.functions[0].argc, 
			parser.functions[0].varc, 
			parser.functions[0].returns);
		break;
	case 0:
		_set_function(main_entry,
			parser.functions[0].content,
			0,
			parser.functions[0].varc,
			true);

		_err = exec();
		result = shared_result;
		break;
	default:
			_err = ERROR_BAD_ARGS;
	}
	return _err;
}
*/
int Calculator::parse_unchecked(const string& input) {

	_err = 0;
	auto split_result = split(input, ' ', 3);
	if (split_result[0] == "setv") {
		return set_variable(split_result[1], atof(split_result[2].c_str()));
	}
	else if (split_result[0] == "setf") {
		auto split_name = split(split_result[1], ':', 2);
		auto split_args = split(split_name[1], ',', 3);
		return _set_function(split_name[0],
			split_result[2],
			atoi(split_args[0].c_str()),
			atoi(split_args[1].c_str()),
			atoi(split_args[2].c_str()) > 0
		);
	}
	else {
		return 1;	// will not set err flag.
	}
}

int Calculator::exec() {
	if (operators.find(main_entry) == operators.end()) {
		return ERROR_NO_MAIN;	
	}
	_init_stack();
	_err = _call(operators[main_entry]);
	
	if (!_err){
		if (!stack_call.empty()) {
			_err = ERROR_STACK_CALL;
		}
	}

	variables[last_answer] = shared_result;
	return _err;
}

void Calculator::_init_operator() {
	set_operator('+', Operator(10, 2, _plus));
	set_operator('-', Operator(10, 2, _minus));
	set_operator('*', Operator(11, 2, _mul));
	set_operator('/', Operator(11, 2, _div));
	set_operator('^', Operator(12, 2, _pow));
	set_operator('=', Operator(2, 2, _self2));
	set_operator('<', Operator(8, 2, _less));
	set_operator('>', Operator(8, 2, _greater));
	set_operator(',', Operator(1, 1, _self));
	set_operator("(", Operator(BRACKET_PRIORITY, 1, _self));
	set_operator("eq", Operator(BRACKET_PRIORITY, 2, _equal));
	set_operator("sqrt", Operator(BRACKET_PRIORITY, 1, _sqrt));
	set_operator("sin", Operator(BRACKET_PRIORITY, 1, _sin));
	set_operator("cos", Operator(BRACKET_PRIORITY, 1, _cos));
	set_operator("tan", Operator(BRACKET_PRIORITY, 1, _tan));
	set_operator("log", Operator(BRACKET_PRIORITY, 1, _log));
	set_operator("log10", Operator(BRACKET_PRIORITY, 1, _log10));
	set_operator("exp", Operator(BRACKET_PRIORITY, 1, _exp));
	set_operator("exp10", Operator(BRACKET_PRIORITY, 1, _exp10));
	set_operator("print", Operator(BRACKET_PRIORITY, 1, _print));
	set_operator("new", Operator(BRACKET_PRIORITY, 1, _self));
	set_operator("index", Operator(BRACKET_PRIORITY, 2, _shift));

	_oper_self = operators["("];
	_oper_new = operators["new"];
	_oper_assign = symple_operators['='];

	operators["lg"] = operators["log10"];
	operators["sqr"] = operators["sqrt"];
}
void Calculator::_init_constant()
{
	set_variable("i", Number(0.0, 1.0));
	set_variable("pi", Number(3.14159265358979));
	set_variable("e", Number(2.71828182845904));
}
void calc::Calculator::_init_stack()
{
	_clear_stack(stack_call);
	stack_frame.clear();
	_frame_pointer = 0;
}

int Calculator::set_variable(const string& name, const Number& value)
{
	variables[name] = value;
	return 0;
}

int Calculator::get_variable(const string& name, Number& val)const
{
	auto num = variables.find(name);
	if (num == variables.end()) {
		return ERROR_BAD_VAR;
	}
	else {
		val = num->second;
		return 0;
	}
}

int calc::Calculator::_set_function(const string& name, const string& instruction, int argc, int varc, bool returns)
{
	functions.push_back(Operator(BRACKET_PRIORITY, argc, varc, returns, instruction));
	operators[name] = &functions.back();
	return 0;
}

void Calculator::set_operator(const string & name, const Operator & oper)
{
	functions.push_back(oper);
	operators[name] = &functions.back();
}

void Calculator::set_operator(char name, const Operator& oper) {
	functions.push_back(oper);
	symple_operators[name] = &functions.back();
}

int calc::Calculator::get_function_signature(const string& name, int & argc, bool & type, bool & returns)const
{
	auto func = operators.find(name);
	if (func == operators.end()) {
		return 1;
	}
	argc = func->second->argc;
	type = func->second->is_string;
	returns = func->second->is_function;
	return 0;
}

vector<string> calc::Calculator::get_all_functions()const
{
	vector<string> function_names;
	for (const auto& a : operators) {
		function_names.push_back(a.first);
	}
	return function_names;
}

int Calculator::get_error() const
{
	return _err;
}

void Calculator::_pushl(MYBIT n) {
	stack_frame.push_back(n);
	if (stack_frame.size() > MAX_STACK) {
		_err = ERROR_STACK_OVERFLOW;
	}
}

void Calculator::_popl(MYBIT& n) {
	n = stack_frame.back();
	stack_frame.pop_back();
}
