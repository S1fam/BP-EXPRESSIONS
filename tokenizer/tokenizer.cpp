#include "tokenizer.hpp"

#include <iostream>
#include <cstdlib>
#include <cctype>

static void exitWithError(const char* message)
{
    std::cerr << message << std::endl;
    std::exit(1);
}

bool Tokenizer::tokenize(const std::string& expression, std::vector<Token>& tokens) {
    size_t i = 0;
    while (i < expression.size())
    {
        char c = expression[i];

        if (std::isspace(static_cast<unsigned char>(c)))
        {
            ++i;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            std::string number;
            while (i < expression.size() && std::isdigit(static_cast<unsigned char>(expression[i])))
            {
                number += expression[i++];
            }
            tokens.push_back({TokenType::Literal, number});
            continue;
        }

        switch (c)
        {
            case '+': tokens.push_back({TokenType::Plus, "+"}); break;
            case '-': tokens.push_back({TokenType::Minus, "-"}); break;
            case '*': tokens.push_back({TokenType::Multiply, "*"}); break;
            case '/': tokens.push_back({TokenType::Divide, "/"}); break;
            default:
                exitWithError("Error: Invalid character in expression.");
        }
        ++i;
    }

    if (!validate(tokens)) exitWithError("Error: Invalid syntax in expression.");
    return true;
}

bool Tokenizer::validate(const std::vector<Token>& tokens) {
    if (tokens.empty()) return false;
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if ((i % 2 == 0 && tokens[i].type != TokenType::Literal) ||
            (i % 2 == 1 && tokens[i].type == TokenType::Literal))
        {
            return false;
        }
    }
    if (tokens.back().type != TokenType::Literal) return false;
    return true;
}