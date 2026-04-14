#include "tokenizer.hpp"

#include <iostream>
#include <cstdlib>
#include <cctype>

static void exitWithError(const char* message)
{
    std::cerr << message << std::endl;
    std::exit(1);
}

bool Tokenizer::tokenize(const std::string& expression, std::vector<Token>& tokens)
{
    size_t i = 0;
    while (i < expression.size())
    {
        char c = expression[i];

        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }

        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            std::string number;
            while (i < expression.size() && std::isdigit(static_cast<unsigned char>(expression[i])))
                number += expression[i++];

            tokens.push_back({ TokenType::Literal, number, std::stod(number) });
            continue;
        }

        switch (c)
        {
            case '+': tokens.push_back({ TokenType::Plus,     "+", std::nullopt }); break;
            case '-': tokens.push_back({ TokenType::Minus,    "-", std::nullopt }); break;
            case '*': tokens.push_back({ TokenType::Multiply, "*", std::nullopt }); break;
            case '/': tokens.push_back({ TokenType::Divide,   "/", std::nullopt }); break;
            default:  exitWithError("Error: Invalid character in expression.");
        }
        ++i;
    }

    if (!validate(tokens)) exitWithError("Error: Invalid syntax in expression.");
    return true;
}

bool Tokenizer::validate(const std::vector<Token>& tokens)
{
    if (tokens.empty()) return false;
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        bool expectLiteral = (i % 2 == 0);
        bool isLiteral     = (tokens[i].type == TokenType::Literal);
        if (expectLiteral != isLiteral) return false;
    }
    return tokens.back().type == TokenType::Literal;
}
