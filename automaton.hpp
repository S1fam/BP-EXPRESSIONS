#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <functional>
#include "token_structure.hpp"

// ── States ────────────────────────────────────────────────────────────────────

enum class State { qs, ql, qp, qm, qn, qd, qf };

std::string stateToStr(State s);

// ── Stack symbol types ────────────────────────────────────────────────────────

enum class NonTerminal { L, P, M, N, D, S };

std::string ntToStr(NonTerminal nt);

struct BottomMarker {};

// A stack symbol is exactly one of: a terminal Token, a NonTerminal marker,
// or the bottom-of-stack sentinel. std::variant handles lifetimes safely.
struct StackSymbol
{
    std::variant<Token, NonTerminal, BottomMarker> data;

    // Optional numeric value bound to this symbol (meaningful for L markers)
    std::optional<double> value;

    bool isTerminal()    const { return std::holds_alternative<Token>(data); }
    bool isNonTerminal() const { return std::holds_alternative<NonTerminal>(data); }
    bool isBottom()      const { return std::holds_alternative<BottomMarker>(data); }

    const Token&       asToken() const { return std::get<Token>(data); }
    NonTerminal        asNT()    const { return std::get<NonTerminal>(data); }
};

// ── Factory helpers (match the old API names) ─────────────────────────────────

inline StackSymbol T(TokenType type, const std::string& val)
{
    return StackSymbol{ Token{type, val, std::nullopt}, std::nullopt };
}

inline StackSymbol NT(NonTerminal nt)
{
    return StackSymbol{ nt, std::nullopt };
}

inline StackSymbol BOTTOM()
{
    return StackSymbol{ BottomMarker{}, std::nullopt };
}

// ── Rule building blocks ──────────────────────────────────────────────────────

struct StackCondition
{
    size_t     depth;   // 1 = deepest NT, 2 = second deepest, …
    NonTerminal symbol;
};

inline StackCondition C(size_t depth, NonTerminal nt) { return {depth, nt}; }

// A Rule describes one transition of the automaton.
//
// Matching:
//   - from_state must equal current state
//   - input (nullopt = epsilon) must match current input token type
//   - every condition must be satisfied (depth-th NT on stack equals symbol)
//   - guard (if set) is an extra predicate evaluated against the whole automaton
//
// Application:
//   - replacements[i] replaces the stack position identified by conditions[i]
//   - if use_deepest_LML is true, a special LML-pattern search drives application
struct Rule
{
    int         id;
    std::string description;

    State from_state;
    State to_state;

    std::optional<TokenType> input;  // nullopt = epsilon

    std::vector<StackCondition>          conditions;
    std::vector<std::vector<StackSymbol>> replacements;

    // Extra guard predicate: called only after basic matching passes.
    // Replaces ad-hoc boolean flags like require_no_M.
    std::function<bool(const struct Automaton&)> guard;

    // When true, application targets the deepest L–M–L triple in the stack
    // rather than using the conditions/replacements arrays directly.
    bool use_deepest_LML = false;
};

// ── Automaton — pure data ─────────────────────────────────────────────────────
//
// The automaton holds only configuration state: current state, stack, and the
// rule table.  The Parser drives the step loop and handles I/O.

struct Automaton
{
    State                   state;
    std::vector<StackSymbol> stack;   // back() = top
    std::vector<Rule>        rules;

    // Re-initialise to start configuration with the given initial state symbol.
    void reset();
};

// Populate the rule table that matches the thesis example (VŘČPHZA, example 2).
// Returns a ready-to-use Automaton.
Automaton buildAutomaton();
