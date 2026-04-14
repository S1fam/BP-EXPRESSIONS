#pragma once

#include <string>
#include <vector>
#include "token_structure.hpp"

class Parser 
{
public:
    bool parse(const std::string& expression);

private:
    std::vector<Token> tokens_;

    void getTokens(const std::string& expression);
    void validateTokens();
};