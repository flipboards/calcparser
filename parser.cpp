
#include <regex>
#include <stack>

#include "parser.h"
#include "error.h"
#include "stringutil.h"

#define IS_INT(x) ((x) - 48 >= 0 && (x) - 48 < 10 || x == '.')


int calc::Parser::parse_line(const string& input){

	string input_formatted = regex_replace(input, regex(" +"), " ");
	
	// set (function or global variable)
	if (input_formatted.size() > 4 && input_formatted.substr(0, 4) == "set ") {
		auto split_results = split(input_formatted, ' ', 3);
		if (split_results.size() < 3) {
			return ERROR_BAD_FUNC;	// global 'set' must has a content!
		}
		
		// is a variable
		if (!regex_match(input_formatted, regex(".*\\("))) {
			if (_check_variable_raw(split_results[1], split_results[2])) {
				return ERROR_BAD_VAR;
			}
			else {
				variables.push_back({ split_results[1], split_results[2] });
			}
		}
		else {
			vector<string> args;
			if (_check_function_raw(split_results[1], split_results[2], args)) {
				return ERROR_BAD_FUNC;
			}
			else {
				return _parse_function(split_results[1], split(split_results[2], ';'), args);
			}
		}
	}

	// not a function
	if (regex_match(input_formatted.begin(), input_formatted.end(), regex(".*="))) {
		return ERROR_BAD_FUNC;	// cannot assign global variable!
	}
	if (input_formatted.back() != ';') {
		input_formatted.push_back(';');
	}
	vector<string> checked_result = split(input_formatted, ';');
	if (_check_function(checked_result)) {
		return ERROR_BAD_FUNC;	// bad function body!
	}
	return _parse_function("main", checked_result, vector<string>());
}

int calc::Parser::parse(const vector<string>& input) {
	string buffer;
	for (const auto& line : input) {
		buffer += line;
	}
	vector<string> input_formatted = split(buffer, ';', -1);
	buffer.clear();

	string* current_function = nullptr;
	for (const auto& line : input_formatted) {

	}
}

int calc::Parser::_parse_function(const string & name, vector<string>& content, const vector<string>& args)
{
	// only used when the function is valid
	// check braket, variables, symbols; replace spaces
	vector<string> locals;
	int returns = 0;

	for (auto pline = content.begin(); pline != content.end();) {

		// setting
		if (regex_match(pline->begin(), pline->end(), regex("set "))) {
			vector<string> set_split;
			if (_check_set(*pline, true, set_split)) {
				return 1;
			}
			locals.push_back(set_split[1]);
			pline = content.erase(pline);
		}

		// not a setting
		else {
			// replace spaces
			regex_replace(pline->begin(), pline->begin(), pline->end(), regex(" +"), "");

			// check variables
			
		}
	}

	for (auto& line : content) {
		// replace minus
		regex_replace(line.begin(), line.begin(), line.end(), regex("\\(\\-"), "\\(0\\-");
	}

	// replace argument varaibles
	for (size_t i = 0; i < args.size(); i++) {
		regex_replace(mycontent, regex(args[i]), "_" + to_string(i));
	}

	Function function;
	function.name = name;
	function.content = join(content, ';');
	function.content.push_back(';');
	function.argc = args.size();
	function.varc = locals.size();
	function.returns = returns;

	functions.push_back(function);
	return 0;
}

int calc::Parser::_check_varname(const string & name)
{
	if (name.empty())return 1;
	if (IS_INT(name[0]) || name[0] == '_')return 1;
	return 0;
}

int calc::Parser::_check_varvalue(const string & value)
{
	if (!IS_INT(value[0]) && value[0] != '-') {
		return 1;
	}
	for (auto v = ++value.begin(); v != value.end(); v++) {
		if (!IS_INT(*v)) {
			return 1;
		}
	}
	return 0;
}

int calc::Parser::_check_function(vector<string>& content) 
{

	// check if barket match.
	stack<char> braket_stack;

	for (const auto& c : name) {
		if (c == ';' || c == '\n') {
			if (!braket_stack.empty() && braket_stack.top() != '}') {
				return ERROR_BAD_FUNC;
			}
		}
		else if (c == '(' || c == '[' || c == '{') {
			braket_stack.push(c);
		}
		else if (c == ')') {
			if (braket_stack.top() != '(')return ERROR_BAD_FUNC;
			braket_stack.pop();
		}
		else if (c == ']') {
			if (braket_stack.top() != ']')return ERROR_BAD_FUNC;
			braket_stack.pop();
		}
		else if (c == '}') {
			if (braket_stack.top() != '}')return ERROR_BAD_FUNC;
			braket_stack.pop();
		}
	}
	if (!braket_stack.empty())return ERROR_BAD_FUNC;
	return 0;
}

int calc::Parser::_check_variable_raw(const string & name, const string& value)
{
	if (_check_varname(name)) {
		return ERROR_BAD_VAR;	// bad variable name!
	}
	else if (_check_varvalue(value)) {
		return ERROR_BAD_VAR;	// cannot convert '' to value!
	}

	return 0;
}

int calc::Parser::_check_function_raw(string & name, const string & value, vector<string>& args)
{
	// check basic grammar problems and arguments

	size_t arg_start = name.find_first_of('(');
	size_t arg_end = name.find_first_of(')');
	if (arg_end == string::npos) {
		return 1;
	}
	args = split(name.substr(arg_start + 1, arg_end), ',', -1);
	name = name.substr(0, arg_start);
	if (_check_varname(name)) {
		return 1;
	}
	for (const auto& arg : args) {
		if (_check_varname(arg)) {
			return 1;
		}
	}

	return _check_function(split(value, ';'));
}

int calc::Parser::_check_set(const string & input, bool is_local, vector<string>& result)
{
	result = split(input, ';');
	if (result.size() > 3 || (is_local && result.size() > 2)) {
		return 1;
	}

	if (_check_varname(result[1])) {
		return 1;
	}
	if (!is_local && _check_varvalue(result[2])) {
		return 1;
	}
	return 0;
}
