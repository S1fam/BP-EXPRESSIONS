/**
 * @file recieve_expression.hpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Definice funkce pro ziskani vyrazu od uzivatele, s instrukcemi a kontrolou prazdneho vstupu
 */

#pragma once

#include <string>
#include <vector>
#include "token_structure.hpp"


void ReceiveExpressionOrExit(std::string& expression);

