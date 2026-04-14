#pragma once

#include <string>
#include <vector>
#include "token_structure.hpp"

class Tokenizer {
public:
    bool tokenize(const std::string& expression, std::vector<Token>& tokens);
    bool validate(const std::vector<Token>& tokens);
};