/**
 * @file parser.hpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Definice parseru pro analyzu a vyhodnoceni aritmetickych vyrazu
 */

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
    Automaton automaton_; // automat
    std::vector<Token> tokens_; // vstup
    size_t input_pos_ = 0; // pozice ve vstupu
    std::optional<double> result_; // vysledek

    // Setup
    void tokenise(const std::string& expression);

    // vsechny indexy zasobniku drzici neterminal, serazeno od dna k vrcholu
    std::vector<size_t> ntIndices() const;

    // index na zasobniku n-teho nejhlubsiho neterminalu (1 = nejblize k vrcholu)
    std::optional<size_t> ntIndex(size_t depth) const;

    std::optional<size_t> deepestLMLPosition() const;

    // podminky pravidel
    bool matchesState(const Rule& rule) const;
    bool matchesInput(const Rule& rule) const;
    bool matchesConditions(const Rule& rule) const;
    bool matchesGuard(const Rule& rule) const;
    const Rule* findMatchingRule() const;

    // krokovani
    bool tryPop();
    void applyRule(const Rule& rule);
    void applyDeepestLML(const Rule& rule);
    void applyConditionsRule(const Rule& rule);

    // vypocet hodnot
    std::optional<double> computeNewLValue(const Rule& rule) const;
    static std::optional<double> compute(double left, TokenType op, double right);

    // vypis
    std::string theoreticalInput() const;
    std::string theoreticalStack() const;
    std::string theoreticalConfig() const;

    std::string practicalInput() const;
    std::string practicalStack() const;
    std::string practicalConfig() const;

    std::string ruleDescription(const Rule& rule) const;

    void printPopStep();
    void printExpandStep(const Rule& rule, const std::string& desc);
};
