#pragma once
/* a parser for in-line parsing. Parser written by
 other languages would be suitable for compile...
 */

#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>

using namespace std;

namespace calc {

	class Parser {
	public:
		int parse_line(const string& input);
		int parse(const vector<string>& input);

		int _parse_function(const string& name, vector<string>& content, const vector<string>& args);
		static int _check_varname(const string&);
		static int _check_varvalue(const string&);
		static int _check_function(vector<string>&);
		static int _check_variable_raw(const string& name, const string& value);
		static int _check_function_raw(string& name, const string& value, vector<string>& args);
		static int _check_set(const string& input, bool is_local, vector<string>& result);

		struct Variable {
			string name;
			string content;
		};
		struct Function {
			string name;
			string content;
			int argc, varc;
			bool returns;
		};
		vector<Function> functions;
		vector<Variable> variables;

	};
}

#endif // !PARSER_H

