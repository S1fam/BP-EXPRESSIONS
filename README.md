# Expression Parser — VŘČPHZA

Konzolová aplikace demonstrující vyhodnocování aritmetických výrazů pomocí
**vstupem řízeného částečně paralelního hlubokého zásobníkového automatu (VŘČPHZA)**.

Automat zpracovává výrazy se čtyřmi operátory (`+`, `-`, `*`, `/`) se správnou
prioritou operací a vypisuje průběh výpočtu krok po kroku ve dvou pohledech —
teoretickém (markery) a praktickém (skutečné hodnoty).

---

## Struktura projektu

```
BP-EXPRESSIONS/
├── main.cpp
├── Makefile
├── README.md
├── automaton/
│   ├── automaton.cpp
│   └── automaton.hpp
├── parser/
│   ├── parser.cpp
│   └── parser.hpp
├── tokenizer/
│   ├── token_structure.hpp
│   ├── tokenizer.cpp
│   └── tokenizer.hpp
└── utils/
    ├── display_welcome.cpp
    ├── display_welcome.hpp
    ├── recieve_expression.cpp
    └── recieve_expression.hpp
```

### Zodpovědnosti modulů

| Modul | Zodpovědnost |
|---|---|
| `automaton` | Datové struktury — zásobníkový symbol, pravidlo, automat. Funkce `buildAutomaton()` sestaví instanci se všemi 14 pravidly. |
| `parser` | Řídí průběh výpočtu: inicializuje automat, střídá pop a expanzní kroky, spravuje hodnotové vazby markerů `L` a vypisuje průběh. |
| `tokenizer` | Lexikální analýza — převede vstupní řetězec na posloupnost tokenů a ověří jejich správné alternování. |
| `utils` | Uvítací obrazovka a načtení výrazu ze standardního vstupu. |
| `main.cpp` | Vstupní bod — propojí moduly, vypíše výsledek. |

---

## Překlad

```bash
make
```

---

## Spuštění

```bash
./parser_app
```

Po spuštění aplikace zadejte aritmetický výraz a stiskněte Enter.
Mezery jsou ignorovány. Stisknutím `Ctrl+C` aplikaci ukončíte.
Prázdný vstup zobrazí nápovědu s příkladem.

---

## Příklad výstupu

Vstup: `3*3-3/3+3`

```
(qs, l*l-l/l+l, S#) --> (qs, 3*3-3/3+3, S#)
|-e (ql, l*l-l/l+l, lL#) --> (ql, 3*3-3/3+3, lL#)  [1: qs,l(1S) -> ql(lL)]
|-p (ql, *l-l/l+l, L#) --> (ql, *3-3/3+3, 3#)
|-e (qn, *l-l/l+l, *NL#) --> (qn, *3-3/3+3, *N3#)  [6: ql,*(1L) -> qn(*NL)]
|-p (qn, l-l/l+l, NL#) --> (qn, 3-3/3+3, N3#)
|-e (ql, l-l/l+l, lL#) --> (ql, 3-3/3+3, l9#)  [7: qn,l(1N,2L) -> ql(l,L)]
|-p (ql, -l/l+l, L#) --> (ql, -3/3+3, 9#)
|-e (qm, -l/l+l, -ML#) --> (qm, -3/3+3, -M9#)  [4: ql,-(1L) -> qm(-ML)]
|-p (qm, l/l+l, ML#) --> (qm, 3/3+3, M9#)
|-e (ql, l/l+l, lLML#) --> (ql, 3/3+3, lLM9#)  [5: qm,l(1M) -> ql(lLM)]
|-p (ql, /l+l, LML#) --> (ql, /3+3, 3M9#)
|-e (qd, /l+l, /DLML#) --> (qd, /3+3, /D3M9#)  [8: ql,/(1L) -> qd(/DL)]
|-p (qd, l+l, DLML#) --> (qd, 3+3, D3M9#)
|-e (ql, l+l, lLML#) --> (ql, 3+3, l1M9#)  [9: qd,l(1D,2L) -> ql(l,L)]
|-p (ql, +l, LML#) --> (ql, +3, 1M9#)
|-e (qp, +l, +PLML#) --> (qp, +3, +P1M9#)  [2: ql,+(1L) -> qp(+PL)]
|-p (qp, l, PLML#) --> (qp, 3, P1M9#)
|-e (ql, l, lLPLML#) --> (ql, 3, lLP1M9#)  [3: qp,l(1P) -> ql(lLP)]
|-p (ql, ε, LPLML#) --> (ql, ε, 3P1M9#)
|-e (qm, ε, LPLML#) --> (qm, ε, 3P1M9#)  [10: ql,ε(1L) -> qm(L)]
|-e (qm, ε, LPL#) --> (qm, ε, 3P8#)  [12: qm,ε(5L,4M,3L) -> qm(ε,L,ε)]
|-e (qp, ε, LPL#) --> (qp, ε, 3P8#)  [11: qm,ε(1L) -> qp(L)]
|-e (qp, ε, L#) --> (qp, ε, 11#)  [13: qp,ε(1L,2P,3L) -> qp(ε,L,ε)]
|-e (qf, ε, #) --> (qf, ε, #)  [14: qp,ε(1L) -> qf(ε)]
Accepted.  Result: 11
```

Každý řádek zobrazuje konfiguraci ve dvou pohledech oddělených `-->`:
- **levý** — teoretický: literály jako `l`, zásobník s markery (`L`, `M`, `P`, `N`, `D`)
- **pravý** — praktický: skutečné číslice, markery `L` nahrazeny aktuální hodnotou

Kroky jsou označeny `⊢e` (expanzní) nebo `⊢p` (pop) s číslem a popisem použitého pravidla.

## Prohlášení o použití AI
Při vývoji této aplikace byly využity nástroje umělé inteligence:
* **Nástroj:** Claude 4.6 Sonnet (Anthropic)
* **Využití:** Tvorba některých funkcí v rámci parseru, nad uvedený funkcemi je poznámka o využití

testovací skript byl následně validován manuálním spouštěním některých scénářů.