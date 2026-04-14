#pragma once

#include <string>
#include <vector>
#include <optional>
#include "token_structure.hpp"
#include "automaton.hpp"

// Parser drives the PPDA step loop.
//
// Responsibilities:
//   - tokenise the input expression
//   - run the step loop (pop / expand)
//   - print step traces in the ⊢p / ⊢e format
//   - propagate numeric values through the automaton so real arithmetic is computed
//   - expose the final result after a successful parse
class Parser
{
public:
    bool parse(const std::string& expression);

    // Meaningful only after a successful parse().
    std::optional<double> result() const { return result_; }

private:
    // ── State ─────────────────────────────────────────────────────────────────

    Automaton             automaton_;
    std::vector<Token>    tokens_;
    size_t                input_pos_ = 0;
    std::optional<double> result_;

    // ── Setup ──────────────────────────────────────────────────────────────────

    void tokenise(const std::string& expression);

    // ── Stack queries ──────────────────────────────────────────────────────────

    // All stack indices that hold NonTerminals, ordered bottom→top.
    std::vector<size_t> ntIndices() const;

    // Stack index of the n-th deepest NonTerminal (1 = topmost).
    std::optional<size_t> ntIndex(size_t depth) const;

    // Position (within ntIndices()) of the bottom-most L–M–L triple.
    std::optional<size_t> deepestLMLPosition() const;

    // ── Rule matching ──────────────────────────────────────────────────────────

    bool matchesState(const Rule& rule)      const;
    bool matchesInput(const Rule& rule)      const;
    bool matchesConditions(const Rule& rule) const;
    bool matchesGuard(const Rule& rule)      const;

    const Rule* findMatchingRule() const;

    // ── Step execution ─────────────────────────────────────────────────────────

    // Pop step: consume a matching terminal; bind Literal value to its L. 
    bool tryPop();

    // Expand step: apply a matched rule.
    void applyRule(const Rule& rule);

    // Apply rule 12 (deepest-LML reduction).
    void applyDeepestLML(const Rule& rule);

    // Apply any rule using its conditions[] / replacements[] arrays.
    void applyConditionsRule(const Rule& rule);

    // ── Value computation ──────────────────────────────────────────────────────

    // Compute the value that should be bound to a new L produced by this rule.
    // Called before stack modification; may also set result_ (rule 14).
    // Returns nullopt when the rule produces no value-bearing L.
    std::optional<double> computeNewLValue(const Rule& rule) const;

    // Perform the arithmetic for one binary operation.
    static std::optional<double> compute(double left, TokenType op, double right);

    // ── Printing ───────────────────────────────────────────────────────────────

    void printConfiguration() const;
    void printPopStep();
    void printExpandStep(const Rule& rule);
};
