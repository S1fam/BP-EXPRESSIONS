#include <iostream>
#include <string>
#include <csignal>
#include <chrono>
#include "display_welcome.hpp"
#include "recieve_expression.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include "token_structure.hpp"

void signalHandler(int signal);

int main()
{
    std::signal(SIGINT, signalHandler); // Set up signal handler for Ctrl+C

    displayWelcome();

    std::string expression; // user input expression
    std::getline(std::cin, expression); // read user input
    ReceiveExpressionOrExit(expression);

    Parser parser;
    if (!parser.parse(expression))
    {
        std::cout << "Parsing error\n";
        return 1;
    }

    std::cout << "ZZZ..." << std::endl;

    return 0;
}


// The signalHandler function allows the program to gracefully exit when the user presses Ctrl+C
// ensuring that any necessary cleanup can be performed before termination.
void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        std::cout << "\n\nGoodbye!" << std::endl;
        exit(0);
    }
}