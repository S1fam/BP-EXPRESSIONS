#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <functional>
#include "token_structure.hpp"

enum class State { qs, ql, qp, qm, qn, qd, qf }; // set of states
std::string stateToStr(State s);

enum class NonTerminal { L, P, M, N, D, S }; // set of non-input (Non-Terminal) stack symbols
std::string ntToStr(NonTerminal nt);

struct BottomMarker {}; // '#' at the bottom of the stack

struct StackSymbol
{
    // Stack symbol is either Terminal (Token/input), Non-Terminal or #
    std::variant<Token, NonTerminal, BottomMarker> data;

    // Optional numeric value bound to L markers on stack
    std::optional<double> value;

    bool isTerminal()    const { return std::holds_alternative<Token>(data); }
    bool isNonTerminal() const { return std::holds_alternative<NonTerminal>(data); }
    bool isBottom()      const { return std::holds_alternative<BottomMarker>(data); }

    const Token& asToken() const { return std::get<Token>(data); } // returns reference to Token if used on Terminal
    NonTerminal  asNT()    const { return std::get<NonTerminal>(data); } // returns NonTerminal if used on Non-Terminal
};

// Factory helpers - help shorten rule definitions

// T(...) creates a Terminal StackSymbol with the given token type and value.
inline StackSymbol T(TokenType type, const std::string& val)
{
    return StackSymbol{ Token{type, val, std::nullopt}, std::nullopt };
}

// NT(...) creates a Non-Terminal StackSymbol with the given symbol and no value.
inline StackSymbol NT(NonTerminal nt)
{
    return StackSymbol{ nt, std::nullopt };
}

// BOTTOM() creates the bottom-of-stack marker.
inline StackSymbol BOTTOM()
{
    return StackSymbol{ BottomMarker{}, std::nullopt };
}

// StackCondition specifies condition on the stack for rule matching
struct StackCondition
{
    size_t depth; // 1 = top of stack, 2 = one below top, etc.
    NonTerminal symbol;
};

inline StackCondition C(size_t depth, NonTerminal nt) { return {depth, nt}; }

struct Rule
{
    int id;
    std::string description; // formal description of the rule for output

    State from_state;
    State to_state;

    std::optional<TokenType> input; // nullopt = epsilon

    std::vector<StackCondition> conditions;
    std::vector<std::vector<StackSymbol>> replacements;

    std::function<bool(const struct Automaton&)> guard; // used for rule 11 (no M left on stack)

    bool use_deepest_LML = false; // true for rule 12 to process the deepest LML
};

struct Automaton
{
    State state;
    std::vector<StackSymbol> stack; // back() = top
    std::vector<Rule> rules;
    void reset();
};

// Populates the Automaton with rules from the thesis example (VŘČPHZA, example 5.2.1).
Automaton buildAutomaton();