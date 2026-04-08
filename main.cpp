#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include "display_welcome.hpp"
#include "recieve_expression.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include "token_structure.hpp"

void signalHandler(int signal);

int main(int argc, char** argv)
{
    std::signal(SIGINT, signalHandler); // Set up signal handler for Ctrl+C

    std::cout << argc << argv[0] << std::endl; // to silence unused parameter warnings
    displayWelcome();

    std::string expression; // user input expression
    std::getline(std::cin, expression); // read user input

    ReceiveExpressionOrExit(expression);

    std::vector<token> tokens;
    if (!tokenizeExpression(expression, tokens)) 
    {
        std::cout << "Error: Invalid character in expression." << std::endl;
        return 1;
    }
    if (!validateTokens(tokens))
    {
        std::cout << "Error: Invalid syntax in expression." << std::endl;
        return 1;
    }
    
    parseExpression(expression, tokens);

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