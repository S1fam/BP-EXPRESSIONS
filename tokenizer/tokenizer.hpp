/**
 * @file tokenizer.hpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Definice tokenizeru pro rozklad aritmetickych vyrazu na tokeny
 */

#pragma once

#include <string>
#include <vector>
#include "token_structure.hpp"

class Tokenizer {
public:
    bool tokenize(const std::string& expression, std::vector<Token>& tokens);
    bool validate(const std::vector<Token>& tokens);
};