#pragma once
#include <string>
#include <vector>

std::vector<std::string> split(const std::string& input, char symbol, int number=-1);
std::string join(const std::vector<std::string>&, char symbol);
