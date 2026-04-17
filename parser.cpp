#include <iostream>
#include <sstream>
#include <algorithm>
#include "parser.hpp"
#include "tokenizer.hpp"

bool Parser::parse(const std::string& expression)
{
    tokenise(expression);

    automaton_ = buildAutomaton();
    input_pos_ = 0;
    result_ = std::nullopt;

    std::cout << theoreticalConfig() << " --> " << practicalConfig() << "\n"; // initial configuration

    constexpr int MAX_STEPS = 10000;
    for (int step = 0; step < MAX_STEPS; ++step)
    {
        if (tryPop()) continue;

        const Rule* rule = findMatchingRule();
        if (!rule) break;

        applyRule(*rule);
    }

    return (automaton_.state == State::qf && input_pos_ == tokens_.size());
}

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

// i = 0 is deepest NonTerminal, used for deepestLML, we want the lowest i where LML appears.
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
        if (automaton_.stack[idx[i]].asNT() == NonTerminal::L &&
            automaton_.stack[idx[i+1]].asNT() == NonTerminal::M &&
            automaton_.stack[idx[i+2]].asNT() == NonTerminal::L)
            return i;
    return std::nullopt;
}

// Rule matching
bool Parser::matchesState(const Rule& rule) const { return automaton_.state == rule.from_state; }
bool Parser::matchesGuard(const Rule& rule) const { return !rule.guard || rule.guard(automaton_); }

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

// return first matching rule in automaton_.rules order
const Rule* Parser::findMatchingRule() const
{
    for (const auto& rule : automaton_.rules)
    {
        if (!matchesState(rule)) continue;
        if (!matchesInput(rule)) continue;
        if (!matchesConditions(rule)) continue;
        if (!matchesGuard(rule)) continue;
        if (rule.use_deepest_LML && !deepestLMLPosition()) continue;
        return &rule;
    }
    return nullptr;
}

// Pop step - consumes a matching terminal, binds Literal value to its L
bool Parser::tryPop()
{
    if (input_pos_ >= tokens_.size()) return false;
    if (automaton_.stack.empty()) return false;

    auto& top = automaton_.stack.back();
    const Token& tok = tokens_[input_pos_];

    if (!top.isTerminal() || top.asToken().type != tok.type) return false;

    automaton_.stack.pop_back();
    ++input_pos_;

    // Bind the numeric value of a consumed Literal to the nearest unbound L. Skip L markers with bound values
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

// Expand step - applies a matched rule
void Parser::applyRule(const Rule& rule)
{
    // Pre-compute the description before the stack is modified
    // useful for rule 12 which formats the description specifically
    std::string desc = ruleDescription(rule);

    // Rule 14 erases the last L (result). We capture its value before modification.
    if (rule.id == 14)
    {
        auto leftIdx = ntIndex(1);
        if (leftIdx && automaton_.stack[*leftIdx].asNT() == NonTerminal::L)
            result_ = automaton_.stack[*leftIdx].value;
    }

    if (rule.use_deepest_LML)
        applyDeepestLML(rule); // rule 12
    else
        applyConditionsRule(rule); // rest of the rules

    automaton_.state = rule.to_state;
    printExpandStep(rule, desc);
}

// Rule 12: qm,ε((k-1)L,kM,(k+1)L) -> qm(ε,L,ε)
void Parser::applyDeepestLML(const Rule&)
{
    auto pos = deepestLMLPosition();
    if (!pos) return;

    auto indices = ntIndices();
    size_t i0 = indices[*pos]; // bottom L (left operand)
    size_t i1 = indices[*pos + 1]; // M marker
    size_t i2 = indices[*pos + 2]; // top L (right operand)

    auto left  = automaton_.stack[i0].value;
    auto right = automaton_.stack[i2].value;

    TokenType operation = TokenType::Minus;

    // Erase NonTerminals highest-index-first.
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i2);
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i1);
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i0);

    StackSymbol result = NT(NonTerminal::L);
    if (left && right)
        result.value = compute(*left, operation, *right);

    automaton_.stack.insert(automaton_.stack.begin() + (ptrdiff_t)i0, result);
}

// Generic conditions/replacements rule.
void Parser::applyConditionsRule(const Rule& rule)
{
    std::optional<double> newLValue = computeNewLValue(rule); // value or nullopt for rules that dont compute new L

    // Build (stack-index, replacement) pairs for each condition
    struct Operation { size_t idx; std::vector<StackSymbol> repl; };
    std::vector<Operation> operations;
    operations.reserve(rule.conditions.size());

    for (size_t i = 0; i < rule.conditions.size(); ++i)
    {
        auto idx = ntIndex(rule.conditions[i].depth);
        if (!idx) 
        { 
            std::cerr << "ERROR: depth " << rule.conditions[i].depth << " not found for rule " << rule.id << "\n"; 
            return; 
        }
        operations.push_back({ *idx, rule.replacements[i] });
    }

    // Sort descending so we take bigger indices (deeper in stack) first
    std::sort(operations.begin(), operations.end(), [](const Operation& a, const Operation& b){ return a.idx > b.idx; });

    if (newLValue) // computeNewLValue returns a value
    {
        for (auto& op : operations)
            for (auto& sym : op.repl) // look for new L in the replacement sequence
                if (sym.isNonTerminal() && sym.asNT() == NonTerminal::L)
                    sym.value = newLValue; // stamp the computed value onto the new L
    }
    else // No computed value, we want to preserve an old value of L if it exists
    {
        for (auto& op : operations)
        {
            if (!automaton_.stack[op.idx].isNonTerminal()) continue;
            if (automaton_.stack[op.idx].asNT() != NonTerminal::L) continue;

            auto oldVal = automaton_.stack[op.idx].value;
            if (!oldVal) continue;

            for (auto& sym : op.repl)
                if (sym.isNonTerminal() && sym.asNT() == NonTerminal::L) // apply to first new L in replacement
                    { 
                        sym.value = oldVal;
                        break; 
                    }
        }
    }

    // Apply replacements in descending index order (from deeper in stack to top)
    for (const auto& op : operations)
    {
        automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)op.idx);
        automaton_.stack.insert(automaton_.stack.begin() + (ptrdiff_t)op.idx, op.repl.rbegin(), op.repl.rend());
    }
}

// Returns numeric value that should be bound to L produced by rule, or nullopt if rule produces no value-bearing L.
std::optional<double> Parser::computeNewLValue(const Rule& rule) const
{
    switch (rule.id)
    {
        // rules 7 and 9: immediate multiply / divide
        case 7:
        case 9:
        {
            auto leftIdx = ntIndex(2); // left operand L on stack is at depth 2

            if (!leftIdx) return std::nullopt;
            if (automaton_.stack[*leftIdx].asNT() != NonTerminal::L) return std::nullopt;

            auto left  = automaton_.stack[*leftIdx].value;
            auto right = (input_pos_ < tokens_.size()) ? tokens_[input_pos_].number
                                                       : std::nullopt; // right operand is next input token
            TokenType operation = (rule.id == 7) ? TokenType::Multiply : TokenType::Divide;

            if (left && right)
                return compute(*left, operation, *right);
            return std::nullopt;
        }

        // addition qp,ε(1L,2P,3L) -> qp(L)
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

        // Rule 14: final acceptance - capture result before L is erased
        case 14:
        {
            auto resultIdx = ntIndex(1);
            if (!resultIdx) return std::nullopt;
            if (automaton_.stack[*resultIdx].asNT() != NonTerminal::L) return std::nullopt;

            const_cast<Parser*>(this)->result_ = automaton_.stack[*resultIdx].value; // store result
            return std::nullopt; // no new L is produced
        }

        // remaining rules produce no value-bearing L, just structural changes, so return nullopt
        default:
            return std::nullopt;
    }
}

// Printing
// Helper: format a double without unnecessary trailing zeros.
static std::string fmtNum(double v)
{
    if (v == (long long)v)
        return std::to_string((long long)v);
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

// Theoretical configuration
// Input: remaining tokens with literals shown as 'l'
std::string Parser::theoreticalInput() const
{
    if (input_pos_ >= tokens_.size()) return "ε";

    std::string out;
    for (size_t i = input_pos_; i < tokens_.size(); ++i)
    {
        const auto& tok = tokens_[i];
        out += (tok.type == TokenType::Literal) ? "l" : tok.value;
    }
    return out;
}

std::string Parser::theoreticalStack() const
{
    std::string out;
    for (int i = (int)automaton_.stack.size() - 1; i >= 0; --i)
    {
        const auto& sym = automaton_.stack[i];
        if (sym.isTerminal()) out += sym.asToken().value;
        else if (sym.isNonTerminal()) out += ntToStr(sym.asNT());
        else out += "#";
    }
    return out;
}

std::string Parser::theoreticalConfig() const
{
    return "(" + stateToStr(automaton_.state) + ", "
               + theoreticalInput() + ", "
               + theoreticalStack() + ")";
}


// Practical configuration
// Input: remaining tokens with their actual values (numbers stay, ops stay)
// Stack: L with a value shows the number L without a value still shows "L"
std::string Parser::practicalInput() const
{
    if (input_pos_ >= tokens_.size()) return "ε";

    std::string out;
    for (size_t i = input_pos_; i < tokens_.size(); ++i)
        out += tokens_[i].value;
    return out;
}

std::string Parser::practicalStack() const
{
    std::string out;
    for (int i = (int)automaton_.stack.size() - 1; i >= 0; --i)
    {
        const auto& sym = automaton_.stack[i];
        if (sym.isBottom())
        {
            out += "#";
        }
        else if (sym.isTerminal())
        {
            out += sym.asToken().value;
        }
        else // NonTerminal
        {
            NonTerminal nt = sym.asNT();
            if (nt == NonTerminal::L)
                out += sym.value ? fmtNum(*sym.value) : "L";
            else
                out += ntToStr(nt);
        }
    }
    return out;
}

std::string Parser::practicalConfig() const
{
    return "(" + stateToStr(automaton_.state) + ", "
               + practicalInput() + ", "
               + practicalStack() + ")";
}

// Step printing
// For rule 12, compute the depths of the L-M-L positions, otherwise print rule.description
std::string Parser::ruleDescription(const Rule& rule) const
{
    if (!rule.use_deepest_LML) return rule.description;

    auto pos = deepestLMLPosition();
    if (!pos) return rule.description;

    auto indices = ntIndices(); // stack indices, ordered bottom -> top
    size_t total = indices.size();

    // Depth from top: depth = total - position_from_bottom (1-based)
    size_t depthL1 = total - *pos; // bottom L - left operand
    size_t depthM  = total - (*pos + 1); // M
    size_t depthL2 = total - (*pos + 2); // top L - right operand

    return "qm,ε(" + std::to_string(depthL1) + "L,"
                   + std::to_string(depthM)  + "M,"
                   + std::to_string(depthL2) + "L) -> qm(ε,L,ε)";
}

void Parser::printPopStep()
{
    std::cout << "|-p " << theoreticalConfig()
              << " --> " << practicalConfig() << "\n";
}

void Parser::printExpandStep(const Rule& rule, const std::string& desc)
{
    std::cout << "|-e " << theoreticalConfig()
              << " --> " << practicalConfig()
              << "  [" << rule.id << ": " << desc << "]\n";
}

std::optional<double> Parser::compute(double left, TokenType op, double right)
{
    switch (op)
    {
        case TokenType::Plus: return left + right;
        case TokenType::Minus: return left - right;
        case TokenType::Multiply: return left * right;
        case TokenType::Divide:
            if (right == 0.0) { std::cerr << "Division by zero\n"; return std::nullopt; }
            return left / right;
        default: return std::nullopt;
    }
}
