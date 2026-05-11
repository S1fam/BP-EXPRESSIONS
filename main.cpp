/**
 * @file main.cpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Vstupni bod programu, volani funkci pro zobrazeni uvodniho textu, ziskani vyrazu od uzivatele a spusteni parseru
 */

#include <iostream>
#include <string>
#include <csignal>
#include "display_welcome.hpp"
#include "recieve_expression.hpp"
#include "parser.hpp"

void signalHandler(int signal);

int main()
{
    std::signal(SIGINT, signalHandler);

    displayWelcome();

    std::string expression;
    std::getline(std::cin, expression);
    ReceiveExpressionOrExit(expression);

    Parser parser;
    if (!parser.parse(expression))
    {
        std::cout << "Parsing failed — expression rejected.\n";
        return 1;
    }

    std::cout << "Accepted.";
    if (parser.result())
        std::cout << "  Result: " << *parser.result();
    std::cout << "\n";

    return 0;
}

// zpracuje SIGINT (Ctrl+C) a korektne ukonci program
void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        std::cout << "\n\nGoodbye!\n";
        std::exit(0);
    }
}
