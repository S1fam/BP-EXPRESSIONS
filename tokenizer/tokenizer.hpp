#pragma once

#include <string>
#include <vector>
#include "token_structure.hpp"

bool tokenizeExpression(const std::string& expression, std::vector<token>& tokens);
bool validateTokens(const std::vector<token>& tokens);