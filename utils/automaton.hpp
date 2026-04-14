#pragma once

#include <string>
#include <vector>
#include <optional>
#include "token_structure.hpp"

enum class State {
    qs, ql, qp, qm, qn, qd, qf
};

enum class NonTerminal {
    L, P, M, N, D, S
};

enum class StackSymbolType {
    Terminal,
    NonTerminal,
    BottomMarker
};

struct StackSymbol {
    StackSymbolType type;
    union {
        std::optional<Token> terminal;
        std::optional<NonTerminal> nonterminal;
    };

    StackSymbol() = delete;

    StackSymbol(StackSymbolType t, std::optional<Token> term, std::optional<NonTerminal> non) : type(t) {
        if (t == StackSymbolType::Terminal) {
            new (&terminal) std::optional<Token>(term);
        } else if (t == StackSymbolType::NonTerminal) {
            new (&nonterminal) std::optional<NonTerminal>(non);
        }
        // BottomMarker does nothing
    }

    ~StackSymbol() {
        if (type == StackSymbolType::Terminal) {
            terminal.~optional<Token>();
        } else if (type == StackSymbolType::NonTerminal) {
            nonterminal.~optional<NonTerminal>();
        }
    }

    StackSymbol(const StackSymbol& other) : type(other.type) {
        if (type == StackSymbolType::Terminal) {
            new (&terminal) std::optional<Token>(other.terminal);
        } else if (type == StackSymbolType::NonTerminal) {
            new (&nonterminal) std::optional<NonTerminal>(other.nonterminal);
        }
    }

    StackSymbol& operator=(const StackSymbol& other) {
        if (this != &other) {
            this->~StackSymbol();
            new (this) StackSymbol(other);
        }
        return *this;
    }

    // Move constructor
    StackSymbol(StackSymbol&& other) noexcept : type(other.type) {
        if (type == StackSymbolType::Terminal) {
            new (&terminal) std::optional<Token>(std::move(other.terminal));
        } else if (type == StackSymbolType::NonTerminal) {
            new (&nonterminal) std::optional<NonTerminal>(std::move(other.nonterminal));
        }
    }

    // Move assignment
    StackSymbol& operator=(StackSymbol&& other) noexcept {
        if (this != &other) {
            this->~StackSymbol();
            new (this) StackSymbol(std::move(other));
        }
        return *this;
    }
};

struct StackCondition {
    size_t depth;
    NonTerminal symbol;
};

struct Rule {
    int id;
    std::string description;
    State from_state;
    State to_state;

    std::optional<TokenType> input; // Sigma or epsilon (std::nullopt)
    std::optional<TokenType> stack_top; // Gamma or epsilon (std::nullopt)

    std::vector<StackCondition> conditions; // Conditions on the stack for the rule to apply
    std::vector<std::vector<StackSymbol>> replacements;

    bool require_no_M = false;
    bool use_deepest_LML = false;
};

inline StackSymbol T(TokenType type, const std::string& val) {
    return StackSymbol{
        StackSymbolType::Terminal,
        Token{type, val},
        std::nullopt
    };
}

inline StackSymbol NT(NonTerminal nt) {
    return StackSymbol{
        StackSymbolType::NonTerminal,
        std::nullopt,
        nt
    };
}

inline StackSymbol BOTTOM() {
    return StackSymbol{
        StackSymbolType::BottomMarker,
        std::nullopt,
        std::nullopt
    };
}

inline StackCondition C(size_t depth, NonTerminal nt) {
    return StackCondition{depth, nt};
}

class Automaton {
public:
    Automaton();

    bool run(const std::vector<Token>& tokens);
private:
    State state_;
    std::vector<StackSymbol> stack_;
    std::vector<Rule> rules_;

    void initRules();

    std::optional<size_t> getNonTerminalIndex(size_t depth) const;

    bool matches(const Rule& rule, std::optional<Token> input_token);
    void applyRule(const Rule& rule);

    bool consumeTerminal(const Token& token);

    void printConfiguration(const std::vector<Token>& input, size_t pos) const; // For debugging

    void printStepRule(const Rule& rule);
    void printStepPop();

    std::optional<size_t> findDeepestLML() const;
};