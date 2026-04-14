#include <iostream>
#include <algorithm>
#include "parser.hpp"
#include "tokenizer.hpp"

// ── Public entry point ────────────────────────────────────────────────────────

bool Parser::parse(const std::string& expression)
{
    tokenise(expression);

    automaton_ = buildAutomaton();
    input_pos_ = 0;
    result_    = std::nullopt;

    printConfiguration();
    std::cout << "\n";

    constexpr int MAX_STEPS = 1000;
    for (int step = 0; step < MAX_STEPS; ++step)
    {
        if (tryPop())          continue;

        const Rule* rule = findMatchingRule();
        if (!rule)             break;

        applyRule(*rule);
    }

    return (automaton_.state == State::qf && input_pos_ == tokens_.size());
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void Parser::tokenise(const std::string& expression)
{
    tokens_.clear();
    Tokenizer tokenizer;
    if (!tokenizer.tokenize(expression, tokens_))
    {
        std::cerr << "Lexical error\n";
        std::exit(1);
    }
}

// ── Stack queries ─────────────────────────────────────────────────────────────

std::vector<size_t> Parser::ntIndices() const
{
    std::vector<size_t> out;
    for (size_t i = 0; i < automaton_.stack.size(); ++i)
        if (automaton_.stack[i].isNonTerminal())
            out.push_back(i);
    return out;
}

// 1-based depth from the top: depth 1 = topmost NonTerminal.
std::optional<size_t> Parser::ntIndex(size_t depth) const
{
    size_t count = 0;
    for (int i = (int)automaton_.stack.size() - 1; i >= 0; --i)
        if (automaton_.stack[i].isNonTerminal())
            if (++count == depth)
                return (size_t)i;
    return std::nullopt;
}

std::optional<size_t> Parser::deepestLMLPosition() const
{
    auto idx = ntIndices();
    for (size_t i = 0; i + 2 < idx.size(); ++i)
        if (automaton_.stack[idx[i]].asNT()   == NonTerminal::L &&
            automaton_.stack[idx[i+1]].asNT() == NonTerminal::M &&
            automaton_.stack[idx[i+2]].asNT() == NonTerminal::L)
            return i;
    return std::nullopt;
}

// ── Rule matching ─────────────────────────────────────────────────────────────

bool Parser::matchesState(const Rule& rule)      const { return automaton_.state == rule.from_state; }
bool Parser::matchesGuard(const Rule& rule)      const { return !rule.guard || rule.guard(automaton_); }

bool Parser::matchesInput(const Rule& rule) const
{
    if (!rule.input)
        return input_pos_ >= tokens_.size();
    if (input_pos_ >= tokens_.size()) return false;
    return tokens_[input_pos_].type == *rule.input;
}

bool Parser::matchesConditions(const Rule& rule) const
{
    for (const auto& cond : rule.conditions)
    {
        auto idx = ntIndex(cond.depth);
        if (!idx || automaton_.stack[*idx].asNT() != cond.symbol) return false;
    }
    return true;
}

const Rule* Parser::findMatchingRule() const
{
    for (const auto& rule : automaton_.rules)
    {
        if (!matchesState(rule))                                   continue;
        if (!matchesInput(rule))                                   continue;
        if (!matchesConditions(rule))                              continue;
        if (!matchesGuard(rule))                                   continue;
        if (rule.use_deepest_LML && !deepestLMLPosition())         continue;
        return &rule;
    }
    return nullptr;
}

// ── Pop step ──────────────────────────────────────────────────────────────────

bool Parser::tryPop()
{
    if (input_pos_ >= tokens_.size()) return false;
    if (automaton_.stack.empty())     return false;

    auto& top = automaton_.stack.back();
    const Token& tok = tokens_[input_pos_];

    if (!top.isTerminal() || top.asToken().type != tok.type) return false;

    automaton_.stack.pop_back();
    ++input_pos_;

    // Bind the numeric value of a consumed Literal to the nearest unbound L.
    // We skip Ls that already carry a value: those belong to a computed result
    // (e.g. from a * or / reduction) and must not be overwritten by the raw input.
    if (tok.type == TokenType::Literal && tok.number)
    {
        for (int i = (int)automaton_.stack.size() - 1; i >= 0; --i)
        {
            auto& sym = automaton_.stack[i];
            if (sym.isNonTerminal() && sym.asNT() == NonTerminal::L && !sym.value)
            {
                sym.value = tok.number;
                break;
            }
        }
    }

    printPopStep();
    return true;
}

// ── Expand step ───────────────────────────────────────────────────────────────

void Parser::applyRule(const Rule& rule)
{
    // Rule 14 erases the last L. Capture its value before modification.
    if (rule.id == 14)
    {
        auto lIdx = ntIndex(1);
        if (lIdx && automaton_.stack[*lIdx].asNT() == NonTerminal::L)
            result_ = automaton_.stack[*lIdx].value;
    }

    if (rule.use_deepest_LML)
        applyDeepestLML(rule);
    else
        applyConditionsRule(rule);

    automaton_.state = rule.to_state;
    printExpandStep(rule);
}

// Rule 12 — reduce the deepest L M L on the stack to a single L.
void Parser::applyDeepestLML(const Rule& /*rule*/)
{
    auto pos = deepestLMLPosition();
    if (!pos) return;

    auto indices = ntIndices();
    size_t i0 = indices[*pos];      // bottom L  (left operand)
    size_t i1 = indices[*pos + 1];  // M marker
    size_t i2 = indices[*pos + 2];  // top    L  (right operand)

    auto left  = automaton_.stack[i0].value;
    auto right = automaton_.stack[i2].value;

    // Find the operator terminal that sits between the bottom-L and the M.
    TokenType opTy = TokenType::Minus;
    for (size_t k = i0 + 1; k < i1; ++k)
        if (automaton_.stack[k].isTerminal())
            { opTy = automaton_.stack[k].asToken().type; break; }

    // Erase the three NonTerminals highest-index-first.
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i2);
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i1);
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i0);

    StackSymbol result = NT(NonTerminal::L);
    if (left && right)
        result.value = compute(*left, opTy, *right);

    automaton_.stack.insert(automaton_.stack.begin() + (ptrdiff_t)i0, result);
}

// Generic conditions/replacements rule.
void Parser::applyConditionsRule(const Rule& rule)
{
    // ── Step 1: Compute the value for any new L marker ───────────────────────
    //
    // This must happen BEFORE we touch the stack, while all depth indices
    // still resolve correctly.

    std::optional<double> newLValue = computeNewLValue(rule);

    // ── Step 2: Build (stack-index, replacement) pairs ───────────────────────

    struct Op { size_t idx; std::vector<StackSymbol> repl; };
    std::vector<Op> ops;
    ops.reserve(rule.conditions.size());

    for (size_t i = 0; i < rule.conditions.size(); ++i)
    {
        auto idx = ntIndex(rule.conditions[i].depth);
        if (!idx) { std::cerr << "ERROR: depth " << rule.conditions[i].depth
                               << " not found for rule " << rule.id << "\n"; return; }
        ops.push_back({ *idx, rule.replacements[i] });
    }

    // Sort descending so each erasure leaves earlier indices intact.
    std::sort(ops.begin(), ops.end(), [](const Op& a, const Op& b){ return a.idx > b.idx; });

    // ── Step 3: Preserve or compute values for any new L in replacements ──────
    //
    // Two cases:
    //   A) A rule that COMPUTES a new value (rules 7, 9, 13): newLValue holds the
    //      result; stamp it onto every new L in the replacements.
    //   B) A rule that MOVES an existing L (e.g. rule 2 replaces an L with +PL,
    //      keeping the old L's meaning): copy the old stack L's value into the
    //      new L in the replacement so the value isn't silently dropped.

    if (newLValue)
    {
        // Case A: computed value.
        for (auto& op : ops)
            for (auto& sym : op.repl)
                if (sym.isNonTerminal() && sym.asNT() == NonTerminal::L)
                    sym.value = newLValue;
    }
    else
    {
        // Case B: value preservation — for each op that replaces an L with
        // a sequence that also contains an L, forward the old value.
        for (auto& op : ops)
        {
            // Is there an L at this stack position?
            if (!automaton_.stack[op.idx].isNonTerminal()) continue;
            if (automaton_.stack[op.idx].asNT() != NonTerminal::L) continue;
            auto oldVal = automaton_.stack[op.idx].value;
            if (!oldVal) continue;

            // Does the replacement contain an L?  If so, forward the value.
            for (auto& sym : op.repl)
                if (sym.isNonTerminal() && sym.asNT() == NonTerminal::L)
                    { sym.value = oldVal; break; } // forward to the first new L only
        }
    }

    // ── Step 4: Apply replacements ───────────────────────────────────────────

    for (const auto& op : ops)
    {
        automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)op.idx);
        automaton_.stack.insert(automaton_.stack.begin() + (ptrdiff_t)op.idx,
                                op.repl.rbegin(), op.repl.rend());
    }
}

// ── Value computation ─────────────────────────────────────────────────────────

// Return the numeric value that should be bound to a new L produced by `rule`,
// or nullopt if the rule doesn't produce a value-bearing L (or values are missing).
// Called before any stack modification.
std::optional<double> Parser::computeNewLValue(const Rule& rule) const
{
    switch (rule.id)
    {
        // ── Rules 7 & 9: immediate multiply / divide ─────────────────────────
        //   Stack: … L … N …   Input: literal
        //   depth-2 = L (left), incoming token = right operand.
        case 7:
        case 9:
        {
            auto lIdx = ntIndex(2);
            if (!lIdx) return std::nullopt;
            if (automaton_.stack[*lIdx].asNT() != NonTerminal::L) return std::nullopt;

            auto left  = automaton_.stack[*lIdx].value;
            auto right = (input_pos_ < tokens_.size()) ? tokens_[input_pos_].number
                                                       : std::nullopt;
            TokenType opTy = (rule.id == 7) ? TokenType::Multiply : TokenType::Divide;
            if (left && right)
                return compute(*left, opTy, *right);
            return std::nullopt;
        }

        // ── Rule 13: addition fold (LPL → L) ─────────────────────────────────
        //   depth-3 = bottom L (left), depth-1 = top L (right).
        case 13:
        {
            auto leftIdx  = ntIndex(3);
            auto rightIdx = ntIndex(1);
            if (!leftIdx || !rightIdx) return std::nullopt;
            if (automaton_.stack[*leftIdx].asNT()  != NonTerminal::L) return std::nullopt;
            if (automaton_.stack[*rightIdx].asNT() != NonTerminal::L) return std::nullopt;

            auto left  = automaton_.stack[*leftIdx].value;
            auto right = automaton_.stack[*rightIdx].value;
            if (left && right)
                return compute(*left, TokenType::Plus, *right);
            return std::nullopt;
        }

        // ── Rule 14: final acceptance — capture result before L is erased ────
        //   depth-1 = the sole remaining L.
        case 14:
        {
            auto lIdx = ntIndex(1);
            if (!lIdx) return std::nullopt;
            if (automaton_.stack[*lIdx].asNT() != NonTerminal::L) return std::nullopt;
            // We return this so applyConditionsRule can stash it, but since the
            // replacement is empty {} the value won't go into the stack.
            // We capture it into result_ here directly instead.
            const_cast<Parser*>(this)->result_ = automaton_.stack[*lIdx].value;
            return std::nullopt; // no new L is produced
        }

        default:
            return std::nullopt;
    }
}

// ── Printing ──────────────────────────────────────────────────────────────────

void Parser::printConfiguration() const
{
    std::cout << "(" << stateToStr(automaton_.state) << ", ";

    if (input_pos_ >= tokens_.size())
        std::cout << "ε";
    else
        for (size_t i = input_pos_; i < tokens_.size(); ++i)
            std::cout << tokens_[i].value;

    std::cout << ", ";

    for (int i = (int)automaton_.stack.size() - 1; i >= 0; --i)
    {
        const auto& sym = automaton_.stack[i];
        if      (sym.isTerminal())    std::cout << sym.asToken().value;
        else if (sym.isNonTerminal()) std::cout << ntToStr(sym.asNT());
        else                          std::cout << "#";
    }

    std::cout << ")";
}

void Parser::printPopStep()
{
    std::cout << "⊢p ";
    printConfiguration();
    std::cout << "\n";
}

void Parser::printExpandStep(const Rule& rule)
{
    std::cout << "⊢e ";
    printConfiguration();
    std::cout << "  [" << rule.id << ": " << rule.description << "]\n";
}

// ── Arithmetic ────────────────────────────────────────────────────────────────

std::optional<double> Parser::compute(double left, TokenType op, double right)
{
    switch (op)
    {
        case TokenType::Plus:     return left + right;
        case TokenType::Minus:    return left - right;
        case TokenType::Multiply: return left * right;
        case TokenType::Divide:
            if (right == 0.0) { std::cerr << "Division by zero\n"; return std::nullopt; }
            return left / right;
        default: return std::nullopt;
    }
}
