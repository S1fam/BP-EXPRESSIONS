#pragma once

#include <string>

enum class TokenType
{
    Literal,
    Plus,
    Minus,
    Multiply,
    Divide
};

struct token
{
    TokenType type;
    std::string value; // for numbers ("123") or operators ("+", "-", "*", "/")
};