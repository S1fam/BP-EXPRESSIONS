/**
 * @file recieve_expression.cpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Implementace funkce pro ziskani vyrazu od uzivatele, s instrukcemi a kontrolou prazdneho vstupu
 */

#include <iostream>
#include "recieve_expression.hpp"

void ReceiveExpressionOrExit(std::string& expression)
{
    while (expression.empty())
    {
        std::cout << "| Try something like: '2 + 3 * 5 - 4 / 2 + 3 - 5'   |" << std::endl;
        std::cout << "| (Whitespaces will be ignored)                     |" << std::endl;
        std::cout << "| (or exit --> ctrl+c)                              |" << std::endl;
        std::getline(std::cin, expression);
    }
}