#include "tokenizer.hpp"

bool tokenizeExpression(const std::string& expression, std::vector<token>& tokens)
{
    size_t i = 0;

    while (i < expression.size())
    {
        char c = expression[i];
        if (std::isspace(c))
        {
            i++;
            continue;
        }

        if (std::isdigit(c)) 
        {
            std::string number;
            while (i < expression.size() && std::isdigit(expression[i]))
            {
                number += expression[i];
                i++;
            }
            tokens.push_back({TokenType::Literal, number});
            continue;
        }
        
        switch (c)
        {
        case '+':
            tokens.push_back({TokenType::Plus, "+"});
            i++;
            break;
        case '-':
            tokens.push_back({TokenType::Minus, "-"});
            i++;
            break;
        case '*':
            tokens.push_back({TokenType::Multiply, "*"});
            i++;
            break;
        case '/':
            tokens.push_back({TokenType::Divide, "/"});
            i++;
            break;
        default:
            return false; // Invalid character
        }
    }
    return true;
}

bool validateTokens(const std::vector<token>& tokens)
{
    if (tokens.empty()) return false;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (i % 2 == 0 && tokens[i].type != TokenType::Literal) // Expecting a literal at even indices
            return false;
        if (i % 2 == 1 && tokens[i].type == TokenType::Literal) // Expecting an operator at odd indices
            return false;
    }

    return true;
}