#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <functional>
#include "token_structure.hpp"

enum class State { qs, ql, qp, qm, qn, qd, qf }; // mnozina stavu
std::string stateToStr(State s);

enum class NonTerminal { L, P, M, N, D, S }; // mnozina neterminalu
std::string ntToStr(NonTerminal nt);

struct BottomMarker {}; // '#' jako konec zasobniku

struct StackSymbol
{
    // Zasobnikovy symbol je terminal nebo neterminal nebo #
    std::variant<Token, NonTerminal, BottomMarker> data;

    // Volitelne ma k sobe vazanou hodnotu (pro L markery)
    std::optional<double> value;

    bool isTerminal() const { return std::holds_alternative<Token>(data); }
    bool isNonTerminal() const { return std::holds_alternative<NonTerminal>(data); }
    bool isBottom() const { return std::holds_alternative<BottomMarker>(data); }

    const Token& asToken() const { return std::get<Token>(data); } // vraci referenci na Token, pokud je pouzito na token
    NonTerminal asNT() const { return std::get<NonTerminal>(data); } // vraci neterminal, pokud je pouzito na neterminal
};

// T(...) vytvori StackSymbol typu Token (terminal), s danym typem tokenu a hodnotou
inline StackSymbol T(TokenType type, const std::string& val)
{
    return StackSymbol{ Token{type, val, std::nullopt}, std::nullopt };
}

// NT(...) vytvori StackSymbol typu NonTerminal
inline StackSymbol NT(NonTerminal nt)
{
    return StackSymbol{ nt, std::nullopt };
}

// BOTTOM() vytvori # pro znaceni dna zasobniku
inline StackSymbol BOTTOM()
{
    return StackSymbol{ BottomMarker{}, std::nullopt };
}

// StackCondition specifikuje podminku na zasobniku pro pravidla, hloubka nt a jaky nt
struct StackCondition
{
    size_t depth; // 1 = nejvrchnejsi neterminal, 2 = druhy nejvrchnejsi...
    NonTerminal symbol;
};

inline StackCondition C(size_t depth, NonTerminal nt) { return {depth, nt}; }

struct Rule
{
    int id;
    std::string description; // popis pravidla pro output

    State from_state;
    State to_state;

    std::optional<TokenType> input; // nullopt = epsilon

    std::vector<StackCondition> conditions;
    std::vector<std::vector<StackSymbol>> replacements;

    std::function<bool(const struct Automaton&)> guard; // pro pravidlo 11 (zadne M na zasobniku)

    bool use_deepest_LML = false; // true pro pravidlo 12
};

struct Automaton
{
    State state;
    std::vector<StackSymbol> stack; // back() = vrchol
    std::vector<Rule> rules;
    void reset();
};

// Vytvori automat z prikladu bakalarske prace (VŘČPHZA, example 5.2.1)
Automaton buildAutomaton();