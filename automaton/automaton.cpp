#include "automaton.hpp"

std::string stateToStr(State s)
{
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

std::string ntToStr(NonTerminal nt)
{
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

void Automaton::reset()
{
    state = State::qs;
    stack.clear();
    stack.push_back(BOTTOM());
    stack.push_back(NT(NonTerminal::S));
}

// Vraci true, pokud neni na zasobniku neterminal M
static bool noMOnStack(const Automaton& a)
{
    for (const auto& sym : a.stack)
        if (sym.isNonTerminal() && sym.asNT() == NonTerminal::M)
            return false;
    return true;
}

Automaton buildAutomaton()
{
    Automaton a;
    a.reset();

    // 1: qs,l(1S) -> ql(lL)
    {
        Rule r;
        r.id = 1;
        r.description = "qs,l(1S) -> ql(lL)";
        r.from_state = State::qs;
        r.to_state = State::ql;
        r.input = TokenType::Literal;
        r.conditions = { C(1, NonTerminal::S) };
        r.replacements = { { T(TokenType::Literal, "l"), NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 2: ql,+(1L) -> qp(+PL)
    {
        Rule r;
        r.id = 2;
        r.description = "ql,+(1L) -> qp(+PL)";
        r.from_state = State::ql;
        r.to_state = State::qp;
        r.input = TokenType::Plus;
        r.conditions = { C(1, NonTerminal::L) };
        r.replacements = { { T(TokenType::Plus, "+"), NT(NonTerminal::P), NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 3: qp,l(1P) -> ql(lLP)
    {
        Rule r;
        r.id = 3;
        r.description = "qp,l(1P) -> ql(lLP)";
        r.from_state = State::qp;
        r.to_state = State::ql;
        r.input = TokenType::Literal;
        r.conditions = { C(1, NonTerminal::P) };
        r.replacements = { { T(TokenType::Literal, "l"), NT(NonTerminal::L), NT(NonTerminal::P) } };
        a.rules.push_back(r);
    }
    // 4: ql,-(1L) -> qm(-ML)
    {
        Rule r;
        r.id = 4;
        r.description = "ql,-(1L) -> qm(-ML)";
        r.from_state = State::ql;
        r.to_state = State::qm;
        r.input = TokenType::Minus;
        r.conditions = { C(1, NonTerminal::L) };
        r.replacements = { { T(TokenType::Minus, "-"), NT(NonTerminal::M), NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 5: qm,l(1M) -> ql(lLM)
    {
        Rule r;
        r.id = 5;
        r.description = "qm,l(1M) -> ql(lLM)";
        r.from_state = State::qm;
        r.to_state = State::ql;
        r.input = TokenType::Literal;
        r.conditions = { C(1, NonTerminal::M) };
        r.replacements = { { T(TokenType::Literal, "l"), NT(NonTerminal::L), NT(NonTerminal::M) } };
        a.rules.push_back(r);
    }
    // 6: ql,*(1L) -> qn(*NL)
    {
        Rule r;
        r.id = 6;
        r.description = "ql,*(1L) -> qn(*NL)";
        r.from_state = State::ql;
        r.to_state = State::qn;
        r.input = TokenType::Multiply;
        r.conditions = { C(1, NonTerminal::L) };
        r.replacements = { { T(TokenType::Multiply, "*"), NT(NonTerminal::N), NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 7: qn,l(1N,2L) -> ql(l,L)
    {
        Rule r;
        r.id = 7;
        r.description = "qn,l(1N,2L) -> ql(l,L)";
        r.from_state = State::qn;
        r.to_state = State::ql;
        r.input = TokenType::Literal;
        r.conditions = { C(1, NonTerminal::N), C(2, NonTerminal::L) };
        r.replacements = { { T(TokenType::Literal, "l") }, { NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 8: ql,/(1L) -> qd(/DL)
    {
        Rule r;
        r.id = 8;
        r.description = "ql,/(1L) -> qd(/DL)";
        r.from_state = State::ql;
        r.to_state = State::qd;
        r.input = TokenType::Divide;
        r.conditions = { C(1, NonTerminal::L) };
        r.replacements = { { T(TokenType::Divide, "/"), NT(NonTerminal::D), NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 9: qd,l(1D,2L) -> ql(l,L)
    {
        Rule r;
        r.id = 9;
        r.description = "qd,l(1D,2L) -> ql(l,L)";
        r.from_state = State::qd;
        r.to_state = State::ql;
        r.input = TokenType::Literal;
        r.conditions = { C(1, NonTerminal::D), C(2, NonTerminal::L) };
        r.replacements = { { T(TokenType::Literal, "l") }, { NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 10: ql,ε(1L) -> qm(L)
    {
        Rule r;
        r.id = 10;
        r.description = "ql,ε(1L) -> qm(L)";
        r.from_state = State::ql;
        r.to_state = State::qm;
        r.input = std::nullopt; // epsilon prechod
        r.conditions = { C(1, NonTerminal::L) };
        r.replacements = { { NT(NonTerminal::L) } };
        a.rules.push_back(r);
    }
    // 11: qm,ε(1L) -> qp(L) - guard: zadne M na zasobniku
    {
        Rule r;
        r.id = 11;
        r.description = "qm,ε(1L) -> qp(L)";
        r.from_state = State::qm;
        r.to_state = State::qp;
        r.input = std::nullopt;
        r.conditions = { C(1, NonTerminal::L) };
        r.replacements = { { NT(NonTerminal::L) } };
        r.guard = noMOnStack;
        a.rules.push_back(r);
    }
    // 12: qm,ε((k-1)L,kM,(k+1)L) -> qm(ε,L,ε) - aplikuje se na nejhlubsi sekvenci LML
    {
        Rule r;
        r.id = 12;
        r.description = "qm,ε(deepest LML) -> qm(L)";
        r.from_state = State::qm;
        r.to_state = State::qm;
        r.input = std::nullopt;
        r.use_deepest_LML = true;
        a.rules.push_back(r);
    }
    // 13: qp,ε(1L,2P,3L) -> qp(ε,L,ε)
    {
        Rule r;
        r.id = 13;
        r.description = "qp,ε(1L,2P,3L) -> qp(ε,L,ε)";
        r.from_state = State::qp;
        r.to_state = State::qp;
        r.input = std::nullopt;
        r.conditions = { C(1, NonTerminal::L), C(2, NonTerminal::P), C(3, NonTerminal::L) };
        r.replacements = { {}, { NT(NonTerminal::L) }, {} };
        a.rules.push_back(r);
    }
    // 14: qp,ε(1L) -> qf(ε)
    {
        Rule r;
        r.id = 14;
        r.description = "qp,ε(1L) -> qf(ε)";
        r.from_state = State::qp;
        r.to_state = State::qf;
        r.input = std::nullopt;
        r.conditions = { C(1, NonTerminal::L) };
        r.replacements = { {} };
        a.rules.push_back(r);
    }

    return a;
}
