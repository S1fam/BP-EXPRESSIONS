#pragma once

#include <string>
#include <vector>
#include <optional>
#include "token_structure.hpp"
#include "automaton.hpp"

class Parser
{
public:
    bool parse(const std::string& expression);
    std::optional<double> result() const { return result_; }

private:
    // State
    Automaton automaton_;
    std::vector<Token> tokens_;
    size_t input_pos_ = 0;
    std::optional<double> result_;

    // Setup
    void tokenise(const std::string& expression);

    // All stack indices that hold NonTerminals, ordered bottom -> top
    std::vector<size_t> ntIndices() const;

    // Stack index of the n-th deepest NonTerminal (1 = topmost)
    std::optional<size_t> ntIndex(size_t depth) const;

    std::optional<size_t> deepestLMLPosition() const;

    // Rule matching
    bool matchesState(const Rule& rule) const;
    bool matchesInput(const Rule& rule) const;
    bool matchesConditions(const Rule& rule) const;
    bool matchesGuard(const Rule& rule) const;
    const Rule* findMatchingRule() const;

    // Step execution
    bool tryPop(); // Pop step: consume a matching terminal; bind Literal value to its L
    void applyRule(const Rule& rule); // Expand step: apply a matched rule
    void applyDeepestLML(const Rule& rule);
    void applyConditionsRule(const Rule& rule); // Apply any rule using its conditions[] / replacements[] arrays.

    // Value computation
    std::optional<double> computeNewLValue(const Rule& rule) const;

    // Perform the arithmetic for one binary operation.
    static std::optional<double> compute(double left, TokenType op, double right);

    // Prints
    std::string theoreticalInput() const;
    std::string theoreticalStack() const;
    std::string theoreticalConfig() const;

    std::string practicalInput() const;
    std::string practicalStack() const;
    std::string practicalConfig() const;

    // Build the description string for a rule
    std::string ruleDescription(const Rule& rule) const;

    void printPopStep();
    void printExpandStep(const Rule& rule, const std::string& desc);
};
