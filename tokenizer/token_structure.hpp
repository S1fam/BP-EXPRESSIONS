/**
 * @file token_structure.hpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Definice struktury pro tokeny
 */

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

    // numericka hodnota, pokud jde o literal, pro operator je nullopt
    std::optional<double> number;
};
