#pragma once

#include <string>
#include <vector>
#include "token_structure.hpp"

bool parseExpression(const std::string& expression, std::vector<token>& tokens);