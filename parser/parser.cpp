/**
 * @file parser.cpp
 * @author Jaroslav Ištvan (xistva03)
 * @date 2026
 * @brief Implementace parseru pro analyzu a vyhodnoceni aritmetickych vyrazu
 */

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

    std::cout << theoreticalConfig() << " --> " << practicalConfig() << "\n"; // pocatecni konfigurace

    constexpr int MAX_STEPS = 10000;
    for (int step = 0; step < MAX_STEPS; ++step)
    {
        if (tryPop()) continue; // pokud lze provest pop, popneme a pokracujeme na dalsi krok

        const Rule* rule = findMatchingRule();
        if (!rule) break; // pokud nenalezneme aplikovatelne pravidlo

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

// i = 0 je nejhlubsi symbol na zasobniku, funkce vraci indexy vsech neterminalu na zasobniku
std::vector<size_t> Parser::ntIndices() const
{
    std::vector<size_t> out;
    for (size_t i = 0; i < automaton_.stack.size(); ++i) 
    {
        if (automaton_.stack[i].isNonTerminal()) out.push_back(i);
    }
    return out;
}

// vyhleda na zasobniku index neterminalu s hloubkou depth (depth = 1 -> nejvyse polozeny neterminal)
std::optional<size_t> Parser::ntIndex(size_t depth) const
{
    size_t count = 1;
    for (int i = (int)automaton_.stack.size() - 1; i >= 0; --i) {
        if (automaton_.stack[i].isNonTerminal()) 
        {
            if (count == depth) return (size_t)i;
            count++;
        }
    }
    return std::nullopt;
}

// vyhleda nejhlubsi sekvenci LML na zasobniku
std::optional<size_t> Parser::deepestLMLPosition() const
{
    auto idx = ntIndices(); // vsechny indexy neterminalu
    for (size_t i = 0; i + 2 < idx.size(); ++i) // prochazime od nejhlubsiho
        if (automaton_.stack[idx[i]].asNT() == NonTerminal::L &&
            automaton_.stack[idx[i+1]].asNT() == NonTerminal::M &&
            automaton_.stack[idx[i+2]].asNT() == NonTerminal::L) 
            {
                return i; // vracime index hlubsiho L
            }
    return std::nullopt;
}

// funkce pro testovani shody stavu automatu a pozadovaneho stavu automatu v pravidle
bool Parser::matchesState(const Rule& rule) const { return automaton_.state == rule.from_state; }

// projde pokud pravidlo nemá guard, nebo pokud projde noMOnStack pro pravidlo 11
bool Parser::matchesGuard(const Rule& rule) const { return !rule.guard || rule.guard(automaton_); }

// true pokud pravidlo ma epsilon vstup a na vstupu automatu je prazdno (epsilon)
// nebo pri shode vstupu automatu a ocekavaneho vstupu pravidla
bool Parser::matchesInput(const Rule& rule) const
{
    if (!rule.input) { return input_pos_ >= tokens_.size(); }
    if (input_pos_ >= tokens_.size()) return false;
    return tokens_[input_pos_].type == *rule.input;
}

// true pokud se na zasobniku v hloubkach dle pravidla vyskytuji ocekavane neterminaly
bool Parser::matchesConditions(const Rule& rule) const
{
    for (const auto& cond : rule.conditions)
    {
        auto idx = ntIndex(cond.depth); // index neterminalu v hloubce depth
        if (!idx || automaton_.stack[*idx].asNT() != cond.symbol) return false; // pokud zde neni ocekavany nt
    }
    return true; // pokud vsechny hloubky obsahuji ocekavane neterminaly
}

// vraci prvni pravidlo odpovidajici podminkam, dle definice automatu mame vzdy max 1 pravidlo
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

// pop krok - zkonzumuje terminal, tedy odstrani jej ze vstupu automatu a vrcholu zasobniku
// a dale pokud popuje ze vstupu literal, svaze jeho hodnotu s nejvyssim markerem L na zasobniku
bool Parser::tryPop()
{
    if (input_pos_ >= tokens_.size()) return false;
    if (automaton_.stack.empty()) return false;

    auto& top = automaton_.stack.back();
    const Token& tok = tokens_[input_pos_];

    if (!top.isTerminal() || top.asToken().type != tok.type) return false;

    automaton_.stack.pop_back();
    ++input_pos_;

    if (tok.type == TokenType::Literal && tok.number) 
    {
        for (int i = (int)automaton_.stack.size() - 1; i >= 0; --i)
        {
            auto& sym = automaton_.stack[i];
            // pokud jde o marker L bez hodnoty
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

// expanzni krok - aplikuje naleznute pravidlo splnujici podminky
void Parser::applyRule(const Rule& rule)
{
    // ulozime popis pravidla pred modifikaci zasobniku, nebot pravidlo 12 po zmene zasobniku zmeni popis
    std::string desc = ruleDescription(rule);

    // pravidlo 14 odebira posledni L ze zasobniku, ulozime si hodnotu pred modifikaci
    if (rule.id == 14)
    {
        auto leftIdx = ntIndex(1);
        if (leftIdx && automaton_.stack[*leftIdx].asNT() == NonTerminal::L)
            result_ = automaton_.stack[*leftIdx].value;
    }

    if (rule.use_deepest_LML) 
    {
        applyDeepestLML(rule); // pravidlo 12
    }
    else 
    {
        applyConditionsRule(rule); // zbytek pravidel
    }

    automaton_.state = rule.to_state;
    printExpandStep(rule, desc);
}

/**
 * pravidlo 12: qm,ε((k-1)L,kM,(k+1)L) -> qm(ε,L,ε)
 * @note Tato funkce byla tvořena za pomoci AI (Claude 4.6 Sonnet).
 */
void Parser::applyDeepestLML(const Rule&)
{
    auto pos = deepestLMLPosition();
    if (!pos) return;

    auto indices = ntIndices();
    size_t i0 = indices[*pos]; // spodni L (levy operand) 
    size_t i1 = indices[*pos + 1]; // M marker
    size_t i2 = indices[*pos + 2]; // vrchni L (pravy operand)

    auto left = automaton_.stack[i0].value;
    auto right = automaton_.stack[i2].value;

    TokenType operation = TokenType::Minus;

    // smazeme neterminaly
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i2);
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i1);
    automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)i0);

    StackSymbol result = NT(NonTerminal::L);
    if (left && right) 
    {
        result.value = compute(*left, operation, *right);
    }

    // vlozime L marker s vysledkem
    automaton_.stack.insert(automaton_.stack.begin() + (ptrdiff_t)i0, result);
}

/**
 * aplikuje expanzni pravidlo
 * @note Tato funkce byla tvorena za pomoci AI (Claude 4.6 Sonnet).
 */
void Parser::applyConditionsRule(const Rule& rule)
{
    // predpocitani hodnoty noveho L (pokud pravidlo tvori novou hodnotu)
    std::optional<double> newLValue = computeNewLValue(rule);

    // sestaveni paru (index neterminalu na zasobniku, hodnota vznikla prepisem tohoto neterminalu) 
    struct Operation { size_t idx; std::vector<StackSymbol> repl; };
    std::vector<Operation> operations;
    operations.reserve(rule.conditions.size());

    // projdeme conditions od nejvyse polozenych neterminalu (nejmensi depth)
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

    // seradime idx sestupne abychom brali nejprve nejvetsi depth
    std::sort(operations.begin(), operations.end(), [](const Operation& a, const Operation& b){ return a.idx > b.idx; });

    if (newLValue)
    {
        for (auto& op : operations)
            for (auto& sym : op.repl) // najdeme nove L v prepisech
                if (sym.isNonTerminal() && sym.asNT() == NonTerminal::L)
                    sym.value = newLValue; // tomuto novemu L priradime vypoctenou hodnotu
    }
    else // pokud pravidlo netvori novou hodnotu, uchovame hodnotu L, pokud existuje
    {
        for (auto& op : operations)
        {
            if (!automaton_.stack[op.idx].isNonTerminal()) continue; // pokud nejde o neterminal
            if (automaton_.stack[op.idx].asNT() != NonTerminal::L) continue; // pokud nejde o L

            auto oldVal = automaton_.stack[op.idx].value;
            if (!oldVal) continue; // pokud L nema hodnotu pro zachovani

            for (auto& sym : op.repl) 
            {
                if (sym.isNonTerminal() && sym.asNT() == NonTerminal::L) // prvni nalezene L v prepisech
                { 
                    sym.value = oldVal; // zde uchovame hodnotu
                    break; 
                }
            }
        }
    }

    // aplikujeme prepisy od nejhlubsich pozic na zasobniku
    for (const auto& op : operations)
    {
        automaton_.stack.erase(automaton_.stack.begin() + (ptrdiff_t)op.idx);
        automaton_.stack.insert(automaton_.stack.begin() + (ptrdiff_t)op.idx, op.repl.rbegin(), op.repl.rend());
    }
}

/**
 * vraci hodnotu ktera by mela byt prirazena L, jenz pravidlo produkuje v ramci prepisu
 * nebo vraci nullopt, pokud pravidlo neprodukuje L
 * @note Tato funkce byla tvorena za pomoci AI (Claude 4.6 Sonnet).
 */
std::optional<double> Parser::computeNewLValue(const Rule& rule) const
{
    switch (rule.id)
    {
        // okamzite nasobeni/deleni
        case 7:
        case 9:
        {
            auto leftIdx = ntIndex(2); // levy operand je nt hloubky 2

            if (!leftIdx) return std::nullopt;
            if (automaton_.stack[*leftIdx].asNT() != NonTerminal::L) return std::nullopt;
            auto left  = automaton_.stack[*leftIdx].value;

            // pravy operand je nasledujici vstupni token 
            auto right = (input_pos_ < tokens_.size()) ? tokens_[input_pos_].number : std::nullopt;

            TokenType operation = (rule.id == 7) ? TokenType::Multiply : TokenType::Divide;

            if (left && right)
            {
                return compute(*left, operation, *right);
            }
            return std::nullopt;
        }

        // scitani qp,ε(1L,2P,3L) -> qp(L)
        case 13:
        {
            auto leftIdx = ntIndex(3); // levy operand je L v hloubce 3 na zasobniku
            auto rightIdx = ntIndex(1);

            if (!leftIdx || !rightIdx) return std::nullopt;
            if (automaton_.stack[*leftIdx].asNT()  != NonTerminal::L) return std::nullopt;
            if (automaton_.stack[*rightIdx].asNT() != NonTerminal::L) return std::nullopt;

            auto left  = automaton_.stack[*leftIdx].value;
            auto right = automaton_.stack[*rightIdx].value;
            if (left && right) 
            {
                return compute(*left, TokenType::Plus, *right);
            }
            return std::nullopt;
        }

        // pravidlo 14: prijeti vysledku - zachyti vysledne L nez jej smazeme
        case 14:
        {
            auto resultIdx = ntIndex(1);
            if (!resultIdx) return std::nullopt;
            if (automaton_.stack[*resultIdx].asNT() != NonTerminal::L) return std::nullopt;

            const_cast<Parser*>(this)->result_ = automaton_.stack[*resultIdx].value; // ulozime vysledek do result_
            return std::nullopt; // zadne nove L neni produkovano
        }

        // zbyvajici pravidla neprodukuji novou hodnotu pro L
        default:
            return std::nullopt;
    }
}

// Vypisy

// formatuje double bez zbytecnych desetinych carek
static std::string fmtNum(double v)
{
    if (v == (long long)v)
        return std::to_string((long long)v);
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

// teoreticka konfigurace, tokeny s literaly se ukazuji jako 'l'
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


// prakticka konfigurace, tokeny na vstupu ukazuji skutecnou hodnotu, 
// L na zasobniku, ktere jsou svazane s hodnotou ukazuji tuto hodnotu, bez hodnoty ukazuji stale 'L'
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
        else
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

// vypis kroku, pro pravidlo 12 pocitame dynamicky hloubky pozic L-M-L, jinak vypisujeme rule.description
std::string Parser::ruleDescription(const Rule& rule) const
{
    if (!rule.use_deepest_LML) return rule.description;

    auto pos = deepestLMLPosition(); // index hlubsiho L, indexovani zde je od dna
    if (!pos) return rule.description;

    auto indices = ntIndices(); // indexy nt na zasobniku
    size_t total = indices.size(); // pocet nt na zasobniku

    size_t depthL1 = total - *pos; // spodni L - levy operand
    size_t depthM = total - (*pos + 1); // M
    size_t depthL2 = total - (*pos + 2); // vrchni L - pravy operand

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
