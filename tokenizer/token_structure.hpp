#pragma once

#include <string>
#include <optional>

enum class TokenType
{
    Literal,
    Plus,
    Minus,
    Multiply,
    Divide
};

struct Token
{
    TokenType type;
    std::string value;

    // Numeric value, populated for Literal tokens
    std::optional<double> number;
};
