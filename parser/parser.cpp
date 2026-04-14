#include <iostream>
#include "parser.hpp"
#include "tokenizer.hpp"
#include "automaton.hpp"

bool Parser::parse(const std::string& expression)
{
    getTokens(expression);
    validateTokens();
    
    Automaton automaton;
    return automaton.run(tokens_);
}

void Parser::getTokens(const std::string& expression) {
    Tokenizer tokenizer;
    if (!tokenizer.tokenize(expression, tokens_)) {
        std::cerr << "Lexical error\n";
        exit(1);
    }
}

void Parser::validateTokens() {
    Tokenizer tokenizer;
    if (!tokenizer.validate(tokens_)) {
        std::cerr << "Syntax error\n";
        exit(1);
    }
}