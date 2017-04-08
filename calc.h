
#pragma once

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stack>
#include <map>
#include <complex>
#include <vector>
#include <list>

using namespace std;

typedef complex<double> Number;
#define MYBIT	unsigned int

namespace calc {

	class Operator {
	public:
		Operator();
		Operator(int Flag, int Argc, Number(*f)(Number*, int*));
		Operator(int Flag, int Argc, unsigned int Varc, bool Returns, const string&);

		inline int priority()const;

		int _priority;
		int argc;
		bool is_string;
		bool is_function;
		unsigned int varc;

		Number(*func)(Number*, int*);
		string instruction;
	};

	class Calculator {
	public:

		Calculator();
		explicit Calculator(const string& LastAnswer);

//		int parse(const string&, Number&);
		int parse_unchecked(const string&);
		int exec();

		int set_variable(const string&, const Number&);
		int get_variable(const string&, Number&)const;
		void set_operator(const string&, const Operator&);	// set functions with braket
		void set_operator(char, const Operator&);	// set operators
		int get_function_signature(const string&, int& argc, bool& type, bool& returns)const;
		vector<string> get_all_functions()const;

		int get_error()const;

	private:
		void _init_operator();
		void _init_constant();
		void _init_stack();
		
		int _set_function(const string& name, const string& instruction, int argc, int varc, bool returns);
		// run instructions from call stack top
		int _run(Number* result);
		// call function by address
		int _call(Operator*);
		int _parse_num(const string&, Number&);
		int _parse_ptr(char option, int offset);
		Number* _deref(int ptr);

		// push into stack
		template<class _Ty>
		void _push(const _Ty& n) {
			stack_frame.resize(stack_frame.size() + sizeof(_Ty) / sizeof(MYBIT));
			*((_Ty*)&stack_frame[stack_frame.size() - sizeof(_Ty) / sizeof(MYBIT)]) = n;
		}
		// pop from stack
		template<class _Ty>
		void _pop(_Ty& n) {
			n = *((_Ty*)&stack_frame[stack_frame.size() - sizeof(_Ty) / sizeof(MYBIT)]);
			stack_frame.resize(stack_frame.size() - sizeof(_Ty) / sizeof(MYBIT));
		}

		inline void _pushl(MYBIT n);
		inline void _popl(MYBIT& n);

		template<class _Ty>
		static void _clear_stack(stack<_Ty>& s) {
			while (!s.empty())s.pop();
		}

		stack<Operator*> stack_call;
		vector<MYBIT> stack_frame;
		vector<MYBIT> memory_heap;

		list<Operator> functions;				// codes
		map<string, Number> variables;			// variable name table
		map<string, Operator*> operators;		// long operator name table
		map<char, Operator*> symple_operators;	// operator name table

		string last_answer;
		const string main_entry = "main";
		MYBIT _frame_pointer, _stack_pointer;

		Number shared_result;		// shared register
		Number tmp_var;				// temp regeister
		Operator* _oper_self;		// left braket
		Operator* _oper_assign;		// assignment
		Operator* _oper_new;		// malloc
		int _err;	// error output
	};

}
#endif // !CALCULATOR_H
