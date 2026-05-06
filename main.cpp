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

// Handle Ctrl+C gracefully.
void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        std::cout << "\n\nGoodbye!\n";
        std::exit(0);
    }
}
