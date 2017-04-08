#include "stringutil.h"

using namespace std;

vector<string> split(const string & str, char symbol, int num)
{
	int cnt = 0;
	vector<string> out;
	int left = 0, right = 0;
	for (right = 0; right < str.size(); right++) {
		if (str[right] == symbol) {
			out.push_back(str.substr(left, right - left));
			left = right + 1;
			cnt++; if (cnt == num)break;
		}
	}
	out.push_back(str.substr(left, str.size() - left));
	return out;
}