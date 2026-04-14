#include <algorithm>
#include <iostream>
#include "automaton.hpp"

static std::string ntToStr(NonTerminal nt) {
    switch (nt) {
        case NonTerminal::L: return "L";
        case NonTerminal::P: return "P";
        case NonTerminal::M: return "M";
        case NonTerminal::N: return "N";
        case NonTerminal::D: return "D";
        case NonTerminal::S: return "S";
    }
    return "?";
}

static std::string stateToStr(State s) {
    switch (s) {
        case State::qs: return "qs";
        case State::ql: return "ql";
        case State::qp: return "qp";
        case State::qm: return "qm";
        case State::qn: return "qn";
        case State::qd: return "qd";
        case State::qf: return "qf";
    }
    return "?";
}

Automaton::Automaton() {
    state_ = State::qs;

    stack_.push_back(BOTTOM());
    stack_.push_back(NT(NonTerminal::S));

    initRules();
}

bool Automaton::run(const std::vector<Token>& tokens) {
    size_t input_pos = 0;
    int steps = 0;

    while (true)
    {
        if (++steps > 1000) break;
        std::optional<Token> current_input =
            (input_pos < tokens.size()) ? std::optional<Token>(tokens[input_pos])
                                       : std::nullopt;

        printConfiguration(tokens, input_pos); // Debugging output
        std::cout << "\n";

        // pop terminal if it matches the input
        if (current_input.has_value() && consumeTerminal(*current_input)) {
            printStepPop();
            input_pos++;
            continue;
        }

        // find a rule that matches the current state, input, and stack conditions
        bool applied = false;

        // try to apply a expansion rule
        for (const auto& rule : rules_) {
            if (matches(rule, current_input)) {
                printStepRule(rule);
                applyRule(rule);
                applied = true;
                break; // only apply one rule per iteration
            }
        }

        if (applied) continue;

        // if no rules matched and we can't consume the input, reject
        break;
    }

    // accept if we ended in the accepting state and consumed all input
    return (state_ == State::qf && input_pos == tokens.size());
}

void Automaton::initRules() {
    { // 1: qs,l(1S) -> ql(lL)
        Rule r;
        r.id = 1;
        r.description = "qs,l(1S) -> ql(lL)";
        r.from_state = State::qs;
        r.to_state = State::ql;
        r.input = TokenType::Literal; // Any literal token can trigger this rule

        r.conditions = { C(1, NonTerminal::S) };

        r.replacements = {
            { T(TokenType::Literal, "l"), NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 2: ql,+(1L) -> qp(+PL)
        Rule r;
        r.id = 2;
        r.description = "ql,+(1L) -> qp(+PL)";
        r.from_state = State::ql;
        r.to_state = State::qp;
        r.input = TokenType::Plus; // Any plus token can trigger this rule

        r.conditions = { C(1, NonTerminal::L) };

        r.replacements = {
            { T(TokenType::Plus, "+"), NT(NonTerminal::P), NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 3: qp,l(1P) -> ql(lLP)
        Rule r;
        r.id = 3;
        r.description = "qp,l(1P) -> ql(lLP)";
        r.from_state = State::qp;
        r.to_state = State::ql;
        r.input = TokenType::Literal;

        r.conditions = { C(1, NonTerminal::P) };

        r.replacements = {
            { T(TokenType::Literal, "l"), NT(NonTerminal::L), NT(NonTerminal::P) }
        };

        rules_.push_back(r);
    }
    { // 4: ql,-(1L) -> qm(-ML)
        Rule r;
        r.id = 4;
        r.description = "ql,-(1L) -> qm(-ML)";
        r.from_state = State::ql;
        r.to_state = State::qm;
        r.input = TokenType::Minus; // Any minus token can trigger this rule

        r.conditions = { C(1, NonTerminal::L) };

        r.replacements = {
            { T(TokenType::Minus, "-"), NT(NonTerminal::M), NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 5: qm,l(1M) -> ql(lLM)
        Rule r;
        r.id = 5;
        r.description = "qm,l(1M) -> ql(lLM)";
        r.from_state = State::qm;
        r.to_state = State::ql;
        r.input = TokenType::Literal;

        r.conditions = { C(1, NonTerminal::M) };

        r.replacements = {
            { T(TokenType::Literal, "l"), NT(NonTerminal::L), NT(NonTerminal::M) }
        };

        rules_.push_back(r);
    }
    { // 6: ql,*(1L) -> qn(*NL)
        Rule r;
        r.id = 6;
        r.description = "ql,*(1L) -> qn(*NL)";
        r.from_state = State::ql;
        r.to_state = State::qn;
        r.input = TokenType::Multiply; // Any multiply token can trigger this rule

        r.conditions = { C(1, NonTerminal::L) };

        r.replacements = {
            { T(TokenType::Multiply, "*"), NT(NonTerminal::N), NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 7: qn,l(1N,2L) -> ql(l,L)
        Rule r;
        r.id = 7;
        r.description = "qn,l(1N,2L) -> ql(l,L)";
        r.from_state = State::qn;
        r.to_state = State::ql;
        r.input = TokenType::Literal;

        r.conditions = {
            C(1, NonTerminal::N),
            C(2, NonTerminal::L)
        };

        r.replacements = {
            { T(TokenType::Literal, "l") }, { NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 8: ql,/(1L) -> qd(/DL)
        Rule r;
        r.id = 8;
        r.description = "ql,/(1L) -> qd(/DL)";
        r.from_state = State::ql;
        r.to_state = State::qd;
        r.input = TokenType::Divide;

        r.conditions = { C(1, NonTerminal::L) };

        r.replacements = {
            { T(TokenType::Divide, "/"), NT(NonTerminal::D), NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 9: qd,l(1D,2L) -> ql(l,L)
        Rule r;
        r.id = 9;
        r.description = "qd,l(1D,2L) -> ql(l,L)";
        r.from_state = State::qd;
        r.to_state = State::ql;
        r.input = TokenType::Literal;

        r.conditions = {
            C(1, NonTerminal::D),
            C(2, NonTerminal::L)
        };

        r.replacements = {
            { T(TokenType::Literal, "l") },
            { NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 10: ql,ε(1L) -> qm(L)
        Rule r;
        r.id = 10;
        r.description = "ql,ε(1L) -> qm(L)";
        r.from_state = State::ql;
        r.to_state = State::qm;
        r.input = std::nullopt; // epsilon

        r.conditions = { C(1, NonTerminal::L) };

        r.replacements = {
            { NT(NonTerminal::L) }
        };

        rules_.push_back(r);
    }
    { // 11: qm,ε(1L) -> qp(L)
        Rule r;
        r.id = 11;
        r.description = "qm,ε(1L) -> qp(L)";
        r.from_state = State::qm;
        r.to_state = State::qp;
        r.input = std::nullopt;

        r.conditions = { C(1, NonTerminal::L) };

        r.replacements = {
            { NT(NonTerminal::L) }
        };

        r.require_no_M = true; // Ensure that there are no pending multiplication operations

        rules_.push_back(r);
    }
    { // 12: qm,ε(1L,2M,3L) -> qm(ε, L, ε)
        Rule r;
        r.id = 12;
        r.description = "qm,ε(1L,2M,3L) -> qm(ε, L, ε)";
        r.from_state = State::qm;
        r.to_state = State::qm;
        r.input = std::nullopt;

        r.use_deepest_LML = true; // This rule will look for the deepest L M L pattern in the stack and replace it with L

        r.conditions = { }; // Conditions are handled by the use_deepest_LML flag

        r.replacements = {
            {}, // L -> ε
            { NT(NonTerminal::L) }, // M -> L
            {}  // L -> ε
        };

        rules_.push_back(r);
    }
    { // 13: qp,ε(1L,2P,3L) -> qp(ε, L, ε)
        Rule r;
        r.id = 13;
        r.description = "qp,ε(1L,2P,3L) -> qp(ε, L, ε)";
        r.from_state = State::qp;
        r.to_state = State::qp;
        r.input = std::nullopt;

        r.conditions = {
            C(1, NonTerminal::L),
            C(2, NonTerminal::P),
            C(3, NonTerminal::L)
        };

        r.replacements = {
            {},
            { NT(NonTerminal::L) },
            {}
        };

        rules_.push_back(r);
    }
    { // 14: qp,ε(1L) -> qf(ε)
        Rule r;
        r.id = 14;
        r.description = "qp,ε(1L) -> qf(ε)";
        r.from_state = State::qp;
        r.to_state = State::qf;
        r.input = std::nullopt;

        r.conditions = { C(1, NonTerminal::L) };

        r.replacements = {
            {}
        };

        rules_.push_back(r);
    }
}

std::optional<size_t> Automaton::getNonTerminalIndex(size_t depth) const {
    size_t count = 0;

    for (int i = stack_.size() - 1; i >= 0; --i) {
        if (stack_[i].type == StackSymbolType::NonTerminal) {
            count++;
            if (count == depth) {
                return i;
            }
        }
    }

    return std::nullopt;
}

bool Automaton::matches(const Rule& rule, std::optional<Token> input_token)
{
    if (state_ != rule.from_state)
        return false;

    // input check
    if (rule.input.has_value()) {
        if (!input_token.has_value()) return false;
        if (rule.input.value() != input_token->type) return false;
    }

    // stack conditions
    for (const auto& cond : rule.conditions) {
        auto idx = getNonTerminalIndex(cond.depth);
        if (!idx.has_value()) return false;

        if (stack_[*idx].nonterminal.value() != cond.symbol)
            return false;
    }

    // M check
    if (rule.require_no_M) {
        for (const auto& s : stack_) {
            if (s.type == StackSymbolType::NonTerminal &&
                s.nonterminal == NonTerminal::M)
                return false;
        }
    }

    // deepest LML check
    if (rule.use_deepest_LML) {
        return findDeepestLML().has_value();
    }

    return true;
}

void Automaton::applyRule(const Rule& rule)
{
    if (rule.use_deepest_LML) {
        auto idx = findDeepestLML();
        if (!idx) return;

        size_t i = *idx;

        // Find the indices of L M L
        std::vector<size_t> nt_indices;
        for (size_t j = 0; j < stack_.size(); ++j) {
            if (stack_[j].type == StackSymbolType::NonTerminal) {
                nt_indices.push_back(j);
            }
        }
        size_t idx1 = nt_indices[i];
        size_t idx2 = nt_indices[i+1];
        size_t idx3 = nt_indices[i+2];

        // Erase from highest index to lowest to avoid index shifting issues
        stack_.erase(stack_.begin() + idx3);
        stack_.erase(stack_.begin() + idx2);
        stack_.erase(stack_.begin() + idx1);

        stack_.insert(stack_.begin() + idx1, NT(NonTerminal::L));

        state_ = rule.to_state;
        return;
    }

    // find indexes of conditions
    std::vector<std::pair<size_t, std::vector<StackSymbol>>> ops;
    for (size_t i = 0; i < rule.conditions.size(); ++i) {
        auto idx = getNonTerminalIndex(rule.conditions[i].depth);
        if (!idx.has_value()) {
            std::cerr << "ERROR: condition index not found\n";
            return;
        }
        ops.push_back({*idx, rule.replacements[i]});
    }

    // sort by index descending to avoid messing up indexes when modifying the stack
    std::sort(ops.begin(), ops.end(),
              [](auto& a, auto& b) { return a.first > b.first; });

    // apply replacements
    for (const auto& [idx, repl] : ops) {
        stack_.erase(stack_.begin() + idx);
        stack_.insert(stack_.begin() + idx, repl.rbegin(), repl.rend());
    }

    // change state
    state_ = rule.to_state;
}

bool Automaton::consumeTerminal(const Token& token)
{
    if (stack_.empty()) return false;

    const auto& top = stack_.back();

    if (top.type == StackSymbolType::Terminal &&
        top.terminal->type == token.type)
    {
        stack_.pop_back();
        return true;
    }

    return false;
}

void Automaton::printConfiguration(const std::vector<Token>& input, size_t pos) const
{
    std::cout << "(" << stateToStr(state_) << ", ";

    if (pos >= input.size()) {
        std::cout << "ε, ";
    } else {
        for (size_t i = pos; i < input.size(); ++i)
            std::cout << input[i].value;
        std::cout << ", ";
    }

    for (int i = stack_.size() - 1; i >= 0; --i) {
        const auto& s = stack_[i];

        if (s.type == StackSymbolType::Terminal && s.terminal.has_value())
            std::cout << s.terminal->value;
        else if (s.type == StackSymbolType::NonTerminal && s.nonterminal.has_value())
            std::cout << ntToStr(*s.nonterminal);
        else
            std::cout << "#";
    }

    std::cout << ")";
}

void Automaton::printStepRule(const Rule& rule)
{
    std::cout << "\n⊢ [" << rule.id << ": " << (rule.description.empty() ? "?" : rule.description) << "]\n";
}

void Automaton::printStepPop()
{
    std::cout << "\n⊢p\n";
}

std::optional<size_t> Automaton::findDeepestLML() const
{
    // go through the stack and find all nonterminal indexes
    std::vector<size_t> nt_indices;

    for (size_t i = 0; i < stack_.size(); ++i) {
        if (stack_[i].type == StackSymbolType::NonTerminal) {
            nt_indices.push_back(i);
        }
    }

    // look for pattern LML in the nonterminals, starting from the bottom of the stack
    for (size_t i = 0; i + 2 < nt_indices.size(); ++i) {
        auto a = stack_[nt_indices[i]].nonterminal;
        auto b = stack_[nt_indices[i+1]].nonterminal;
        auto c = stack_[nt_indices[i+2]].nonterminal;

        if (a == NonTerminal::L &&
            b == NonTerminal::M &&
            c == NonTerminal::L)
        {
            return nt_indices[i]; // index of the first L in the pattern
        }
    }

    return std::nullopt;
}