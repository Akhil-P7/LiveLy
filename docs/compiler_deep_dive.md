# LiveLy Compiler — Deep Dive: Theory & Implementation

> A comprehensive explanation of how the Lexer, Parser, AST, and Semantic Analyzer
> work — covering both the Computer Science theory and the actual LiveLy implementation.

---

## Table of Contents

1. [Overview: The Compiler Pipeline](#1-overview-the-compiler-pipeline)
2. [Phase 1 — Lexical Analysis (Lexer)](#2-phase-1--lexical-analysis-lexer)
3. [Phase 2 — Parsing (Parser)](#3-phase-2--parsing-parser)
4. [Phase 3 — Abstract Syntax Tree (AST)](#4-phase-3--abstract-syntax-tree-ast)
5. [Phase 4 — Semantic Analysis](#5-phase-4--semantic-analysis)
6. [How They All Connect](#6-how-they-all-connect)

---

## 1. Overview: The Compiler Pipeline

### Theory

A compiler transforms human-readable source code into a form that can ultimately be
executed by a machine. This transformation happens in **stages** (also called **phases**),
where each stage takes the output of the previous stage and refines it further.

The classic compiler pipeline:

```
Source Code  →  Lexer  →  Parser  →  AST  →  Semantic Analyzer  →  [Backend...]
     |              |          |        |              |
  raw text      tokens     parse     tree        validated
  (string)      (list)     tree    (in memory)     tree
```

Each phase has a single, well-defined responsibility:

| Phase | Input | Output | Responsibility |
|-------|-------|--------|---------------|
| Lexer | Raw source string | Token stream | Break text into meaningful units |
| Parser | Token stream | AST (tree) | Enforce grammar rules / structure |
| AST | (data structure) | (data structure) | Represent program structure in memory |
| Semantic Analyzer | AST | Validated AST | Enforce meaning rules (types, scope) |

### LiveLy Implementation

In LiveLy, the pipeline is orchestrated by `main.cpp`:

```cpp
std::string source = readFile(argv[1]);     // raw text
Lexer lexer(source);
std::vector<Token> tokens = lexer.tokenize(); // → tokens
Parser parser(tokens);
auto ast = parser.parse();                    // → AST
ASTPrinter::printProgram(ast);                // → visual output
SemanticAnalyzer analyzer;
analyzer.analyze(ast);                        // → validated
```

Each component lives in its own directory and only depends on the component before it.

---

## 2. Phase 1 — Lexical Analysis (Lexer)

### Theory: What Is Lexical Analysis?

Lexical analysis (or **scanning**) is the process of converting a stream of characters
into a stream of **tokens**. A token is the smallest meaningful unit in a programming
language — like a word in a natural language sentence.

Consider this LiveLy line:
```
bind x:int is 10;
```

A human sees six distinct pieces: the keyword `bind`, a name `x`, a colon `:`,
a type `int`, the assignment word `is`, a number `10`, and a semicolon `;`.
The Lexer's job is to produce exactly this decomposition.

**Key concepts:**

- **Token**: A `(type, value, position)` tuple. Example: `(BIND, "bind", line 1, col 1)`.
- **Keyword**: A reserved word that has special meaning (`bind`, `emit`, `check`, etc.).
- **Identifier**: A user-defined name (`x`, `myVar`, `clamp`).
- **Literal**: A constant value embedded in source code (`10`, `alive`).
- **Lookahead**: Peeking at the next character without consuming it — needed for
  multi-character tokens like `>=` or `!=`.

**Finite State Machine (FSM) model:**

Theoretically, a lexer is a Deterministic Finite Automaton (DFA). At each step it reads
one character, transitions to a new state, and decides whether it has completed a token.
The states correspond to "reading a number", "reading an identifier", "reading an operator", etc.

```
START ──[letter]──→ IDENTIFIER state ──[non-alnum]──→ emit IDENTIFIER token
      ──[digit]───→ NUMBER state     ──[non-digit]──→ emit INT_LITERAL token
      ──[+]───────→ emit PLUS token
      ──[>]───────→ GREATER state    ──[=]──→ emit GREATER_EQUAL token
                                     ──[other]──→ emit GREATER token
```

### LiveLy Implementation

**Files:** `src/lexer/token.h`, `src/lexer/lexer.h`, `src/lexer/lexer.cpp`

#### Token Definition (`token.h`)

```cpp
enum class TokenType {
    // Keywords
    BIND, IS, EMIT, CHECK, OTHERWISE, CYCLE, FORGE, YIELD,
    // Types
    TYPE_INT, TYPE_BOOL,
    // Literals
    INT_LITERAL, BOOL_LITERAL,
    // Identifier
    IDENTIFIER,
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE,
    GREATER, LESS, GREATER_EQUAL, LESS_EQUAL, EQUAL, NOT_EQUAL,
    // Assignment
    ASSIGN,  // "is"
    // Symbols
    COLON, SEMICOLON, COMMA, LPAREN, RPAREN, LBRACE, RBRACE,
    // Special
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};
```

LiveLy has 30 distinct token types. Each token stores its position (line + column)
for error reporting later in the pipeline.

#### The Scanner Engine (`lexer.cpp`)

The `Lexer` class maintains three pieces of state:
- `position` — current index into the source string
- `line` — current line number (for error messages)
- `column` — current column number

The `tokenize()` method implements the main DFA loop:

```cpp
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;  // handle trailing whitespace

        char c = currentChar();

        if (std::isalpha(c))       → readIdentifierOrKeyword()
        else if (std::isdigit(c))  → readNumber()
        else if (std::ispunct(c))  → readOperator() or readSymbol()
        else                       → throw error
    }

    tokens.push_back(EOF token);
    return tokens;
}
```

**Keyword detection** uses a hash map for O(1) lookup:

```cpp
static std::unordered_map<std::string, TokenType> keywords = {
    {"bind", TokenType::BIND},
    {"is",   TokenType::ASSIGN},
    {"emit", TokenType::EMIT},
    // ...
};
```

When `readIdentifierOrKeyword()` finishes reading an alphanumeric sequence like `bind`,
it checks the map. If found → keyword token. If not found → identifier token.

**Multi-character operators** use `peek()` (lookahead):

```cpp
case '>':
    if (peek() == '=') {
        advance(); advance();
        return Token(GREATER_EQUAL, ">=", ...);
    }
    advance();
    return Token(GREATER, ">", ...);
```

#### Example: Tokenizing `bind x:int is 10;`

```
Position 0: 'b' → isalpha → readIdentifierOrKeyword()
  reads "bind" → found in keywords → Token(BIND, "bind", 1, 1)

Position 5: 'x' → isalpha → readIdentifierOrKeyword()
  reads "x" → NOT in keywords → Token(IDENTIFIER, "x", 1, 6)

Position 6: ':' → ispunct → readSymbol()
  → Token(COLON, ":", 1, 7)

Position 7: 'i' → isalpha → readIdentifierOrKeyword()
  reads "int" → found in keywords → Token(TYPE_INT, "int", 1, 8)

Position 11: 'i' → isalpha → readIdentifierOrKeyword()
  reads "is" → found in keywords → Token(ASSIGN, "is", 1, 12)

Position 14: '1' → isdigit → readNumber()
  reads "10" → Token(INT_LITERAL, "10", 1, 15)

Position 16: ';' → ispunct → readSymbol()
  → Token(SEMICOLON, ";", 1, 17)

End of input → Token(END_OF_FILE, "", 1, 18)
```

**Output:** 8 tokens from 17 characters.

---

## 3. Phase 2 — Parsing (Parser)

### Theory: What Is Parsing?

Parsing (or **syntactic analysis**) takes the flat token stream and determines whether
it conforms to the language's **grammar** — the set of rules that define valid programs.
The output is a tree structure that represents the program's syntactic structure.

**Context-Free Grammar (CFG):**

Every programming language is defined by a grammar. For LiveLy:

```
program       → statement*
statement     → varDecl | assignment | emit | ifStmt | loop | function | return
varDecl       → BIND IDENTIFIER COLON type ASSIGN expression SEMICOLON
assignment    → IDENTIFIER ASSIGN expression SEMICOLON
emit          → EMIT expression SEMICOLON
ifStmt        → CHECK LPAREN expression RPAREN block (OTHERWISE block)?
loop          → CYCLE LPAREN expression RPAREN block
function      → FORGE IDENTIFIER LPAREN params RPAREN (COLON type)? block
return        → YIELD expression SEMICOLON

expression    → equality
equality      → comparison ( (EQUAL | NOT_EQUAL) comparison )*
comparison    → term ( (GREATER | LESS | GREATER_EQUAL | LESS_EQUAL) term )*
term          → factor ( (PLUS | MINUS) factor )*
factor        → primary ( (MULTIPLY | DIVIDE) primary )*
primary       → INT_LITERAL | BOOL_LITERAL | IDENTIFIER | LPAREN expression RPAREN
```

**Recursive Descent Parsing:**

LiveLy uses a **recursive descent parser** — one of the most common and intuitive parsing
techniques. Each grammar rule becomes a function:

```
parseStatement()  → dispatches to parseVarDecl(), parseEmit(), etc.
parseExpression() → calls parseEquality()
parseEquality()   → calls parseComparison(), handles ==, !=
parseComparison() → calls parseTerm(), handles >, <, >=, <=
parseTerm()       → calls parseFactor(), handles +, -
parseFactor()     → calls parsePrimary(), handles *, /
parsePrimary()    → handles literals, variables, grouped expressions
```

**Operator Precedence:**

The call hierarchy naturally enforces operator precedence. Functions called *deeper*
bind *tighter*:

```
Weakest   ==  !=           (parseEquality)
  ↓       >  <  >=  <=     (parseComparison)
  ↓       +  -             (parseTerm)
Tightest  *  /             (parseFactor)
```

So `2 + 3 * 4` is parsed as `2 + (3 * 4)` because `parseTerm` calls `parseFactor`
first, which grabs `3 * 4` before `parseTerm` sees the `+`.

### LiveLy Implementation

**Files:** `src/parser/parser.h`, `src/parser/parser.cpp`

#### Core Navigation Methods

```cpp
Token peek() const;      // Look at current token without consuming
Token advance();         // Consume current token, return it
bool check(TokenType);   // Does current token match this type?
bool match(TokenType);   // If current matches, consume and return true
Token consume(TokenType, string errorMessage);  // Expect this type or throw
```

#### Statement Dispatch (`parseStatement`)

```cpp
std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::BIND))       return parseVarDecl();
    if (match(TokenType::EMIT))       return parseEmit();
    if (match(TokenType::CHECK))      return parseIf();
    if (match(TokenType::CYCLE))      return parseLoop();
    if (match(TokenType::FORGE))      return parseFunction();
    if (match(TokenType::YIELD))      return parseReturn();
    if (check(TokenType::IDENTIFIER)) return parseAssignment();
    throw runtime_error("Unexpected statement ...");
}
```

The first token of each statement uniquely identifies its type.  This is called
the **FIRST set** in grammar theory — it's what makes LiveLy's grammar LL(1)
(parseable with just one token of lookahead).

#### Expression Parsing Example

For the expression `2 + 3 * (4 - 1)`:

```
parseExpression()
  └→ parseEquality()
       └→ parseComparison()
            └→ parseTerm()
                 ├→ parseFactor()
                 │    └→ parsePrimary() → Literal(2)
                 │  [sees +, loops]
                 └→ parseFactor()
                      ├→ parsePrimary() → Literal(3)
                      │  [sees *, loops]
                      └→ parsePrimary()
                           └→ [sees '('] → parseExpression() recursively
                                 └→ parseTerm()
                                      ├→ Literal(4)
                                      │  [sees -]
                                      └→ Literal(1)
                                      → BinaryExpr("-", 4, 1)
                           → BinaryExpr("*", 3, (4-1))
                 → BinaryExpr("+", 2, (3*(4-1)))
```

#### Function Declaration Parsing

For `forge clamp(n:int, limit:int): int { ... }`:

```cpp
Token name = consume(IDENTIFIER);  // "clamp"
consume(LPAREN);
// Parse parameter list
do {
    Token paramName = consume(IDENTIFIER);  // "n", "limit"
    consume(COLON);
    Token paramType = advance();            // "int", "int"
    parameters.push_back({paramName, paramType});
} while (match(COMMA));
consume(RPAREN);

// Optional return type
if (match(COLON)) {
    returnType = advance();  // "int"
}

consume(LBRACE);
body = parseBlock();  // recursively parse statements until '}'
```

---

## 4. Phase 3 — Abstract Syntax Tree (AST)

### Theory: What Is an AST?

The Abstract Syntax Tree is the central data structure of a compiler. It represents
the **logical structure** of a program, stripped of syntactic noise (semicolons,
parentheses, keywords). Every node in the tree is either a **statement** (does something)
or an **expression** (produces a value).

**Parse Tree vs AST:**

A parse tree mirrors the grammar exactly — every production rule becomes a node.
An AST simplifies this by removing unnecessary intermediate nodes:

```
Parse tree for "2 + 3":        AST for "2 + 3":
     term                        BinaryExpr(+)
    / | \                         /         \
factor + factor              Literal(2)  Literal(3)
  |       |
  2       3
```

The AST only keeps what matters for execution.

**Node Hierarchy (typical):**

```
ASTNode (abstract base)
  ├── Expression (produces a value)
  │    ├── LiteralExpr   — e.g., 42, "alive"
  │    ├── VariableExpr  — e.g., x, myVar
  │    └── BinaryExpr    — e.g., a + b, x >= y
  └── Statement (performs an action)
       ├── VarDecl       — bind x:int is 10;
       ├── Assignment    — x is 20;
       ├── EmitStmt      — emit x;
       ├── IfStmt        — check (...) { } otherwise { }
       ├── LoopStmt      — cycle (...) { }
       ├── ReturnStmt    — yield value;
       └── FunctionDecl  — forge name(...) { }
```

### LiveLy Implementation

**Files:** `src/ast/ast.h`, `src/ast/ast_printer.h`

#### Node Classes (`ast.h`)

Every node uses `std::unique_ptr` for ownership — when a parent is destroyed, its
children are automatically deallocated. This prevents memory leaks without a garbage
collector.

```cpp
class BinaryExpr : public Expression {
public:
    std::string op;                    // "+", "-", ">=", etc.
    std::unique_ptr<Expression> left;  // left operand (owned)
    std::unique_ptr<Expression> right; // right operand (owned)
};

class IfStmt : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> thenBranch;
    std::vector<std::unique_ptr<Statement>> elseBranch;
};

class FunctionDecl : public Statement {
public:
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters;  // (name, type)
    std::string returnType;
    std::vector<std::unique_ptr<Statement>> body;
};
```

#### AST Printer (`ast_printer.h`)

To verify the tree is correct, the AST Printer walks every node using `dynamic_cast`
to identify its concrete type, then prints it with indentation proportional to depth.

For `hello.lv`, the printer produces:

```
[0] VarDecl: x : int
      +-- Literal: 10
[1] VarDecl: y : int
      +-- BinaryOp: +
            +-- Left:  Literal: 2
            +-- Right: BinaryOp: *
                  +-- Left:  Literal: 3
                  +-- Right: BinaryOp: -
                        +-- Left:  Literal: 4
                        +-- Right: Literal: 1
[2] VarDecl: aliveflag : bool
      +-- Literal: alive
[3] Assignment: x
      +-- BinaryOp: /
            +-- Left:  Variable: x
            +-- Right: Variable: y
[4] IfStmt (check):
      +-- Condition: BinaryOp: >=  (Variable: x, Literal: 5)
      +-- Then:      Emit → Variable: x
      +-- Otherwise: Emit → Literal: 0
[5] LoopStmt (cycle):
      +-- Condition: BinaryOp: !=  (Variable: x, Literal: 0)
      +-- Body:      Assignment: x → BinaryOp: - (Variable: x, Literal: 1)
[6] FunctionDecl: clamp -> int
      +-- Params:  n:int, limit:int
      +-- Body:    IfStmt → yield n / yield limit
```

This visualisation proves:
- Operator precedence is correct (`*` binds tighter than `+`)
- Parenthesized grouping works (`(4 - 1)` is a subtree)
- Control flow nesting is preserved (yield inside if, inside function)

---

## 5. Phase 4 — Semantic Analysis

### Theory: What Is Semantic Analysis?

The Parser ensures the program is **syntactically** valid — it follows the grammar rules.
But syntax alone doesn't guarantee **meaning**. Semantic analysis checks that the program
makes logical sense.

**Three categories of semantic errors:**

| Category | Example | Why Syntax Can't Catch It |
|----------|---------|--------------------------|
| **Type Errors** | `bind x:int is alive;` | Grammar allows any expr after `is` |
| **Scope Errors** | `emit y;` (y never declared) | Grammar allows any identifier |
| **Declaration Errors** | Two `bind x:int` in same scope | Grammar has no memory |

**The Symbol Table:**

The central data structure for semantic analysis is the **symbol table** — a mapping
from variable names to their types and scope information. When processing a declaration,
we add to the table. When processing a usage, we look up from the table.

**Scoping:**

Most languages support nested scopes — a variable declared inside a function is not
visible outside. This is modeled with a **scope stack**:

```
Global Scope:  { x → int, aliveflag → bool, clamp → func }
  │
  └─ Function "clamp" Scope:  { n → int, limit → int }
```

When looking up a variable, we search from the innermost scope outward. This implements
**lexical scoping** — the foundational scoping rule used by C, Java, Python, and LiveLy.

**Type Inference for Expressions:**

To check `bind x:int is 2 + 3;`, the analyzer must determine that `2 + 3` evaluates
to type `int`. This requires recursive type inference:

```
typeOf(2 + 3)
  = typeOf(BinaryExpr("+", Literal(2), Literal(3)))
  = check that typeOf(Literal(2)) == "int"  ✓
    check that typeOf(Literal(3)) == "int"  ✓
    "+" requires int operands → returns "int"
```

**Comparison operators** are special — they take `int` operands but return `bool`:

```
typeOf(x >= 5)
  = typeOf(BinaryExpr(">=", Variable(x), Literal(5)))
  = lookup(x) → "int"
    typeOf(Literal(5)) → "int"
    ">=" requires int operands → returns "bool"
```

### LiveLy Implementation

**Files:** `src/semantic/semantic.h`, `src/semantic/semantic.cpp`

#### Scope Stack

```cpp
class SemanticAnalyzer {
    std::vector<std::unordered_map<std::string, std::string>> scopes;
    std::string currentReturnType;  // tracks expected return type in functions
};
```

Instead of a flat symbol table, LiveLy uses a **stack of scopes**. This correctly
handles function parameters being visible only inside the function body.

```cpp
void SemanticAnalyzer::declare(const std::string& name, const std::string& type) {
    auto& current = scopes.back();
    if (current.find(name) != current.end()) {
        error("Variable already declared in this scope: " + name);
    }
    current[name] = type;
}

std::string SemanticAnalyzer::lookup(const std::string& name) {
    for (int i = scopes.size() - 1; i >= 0; --i) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end()) return it->second;
    }
    error("Undefined variable: " + name);
}
```

`lookup()` walks **inside-out** — checking the current scope first, then progressively
outer scopes. This means a local variable with the same name as a global one will
**shadow** the global — exactly like in C/C++.

#### Statement Analysis

Each statement type triggers specific checks:

**Variable Declaration (`bind x:int is 10;`):**
```cpp
if (auto var = dynamic_cast<VarDecl*>(stmt)) {
    std::string valueType = analyzeExpression(var->value.get());
    if (valueType != var->type) {
        error("Type mismatch: expected " + var->type + ", got " + valueType);
    }
    declare(var->name, var->type);
}
```
1. Infer the type of the right-hand expression.
2. Compare it against the declared type.
3. If they match, register the variable in the current scope.

**Function Declaration (`forge clamp(n:int, limit:int): int { ... }`):**
```cpp
if (auto fn = dynamic_cast<FunctionDecl*>(stmt)) {
    declare(fn->name, "func");           // register function in current scope
    pushScope();                          // new scope for function body
    for (auto& param : fn->parameters)
        declare(param.first, param.second); // register params
    currentReturnType = fn->returnType;   // track for yield-checking
    for (auto& s : fn->body)
        analyzeStatement(s.get());        // analyze each statement in body
    popScope();                           // exit function scope
}
```

This ensures function parameters are visible inside the function but not outside,
and that `yield` statements return the correct type.

#### Expression Type Inference

```cpp
std::string SemanticAnalyzer::analyzeExpression(Expression* expr) {
    if (auto lit = dynamic_cast<LiteralExpr*>(expr)) {
        if (lit->value == "alive" || lit->value == "dead") return "bool";
        return "int";
    }
    else if (auto var = dynamic_cast<VariableExpr*>(expr)) {
        return lookup(var->name);  // consult the scope stack
    }
    else if (auto bin = dynamic_cast<BinaryExpr*>(expr)) {
        std::string left  = analyzeExpression(bin->left.get());
        std::string right = analyzeExpression(bin->right.get());

        if (op is arithmetic)  → require int + int → return int
        if (op is comparison)  → require int + int → return bool
        if (op is equality)    → require matching  → return bool
    }
}
```

#### What Gets Caught

| LiveLy Code | Error Detected |
|-------------|---------------|
| `bind x:int is alive;` | Type mismatch: expected int, got bool |
| `emit y;` | Undefined variable: y |
| `bind x:int is 10; bind x:int is 20;` | Variable already declared: x |
| `check (10) { ... }` | Condition must be bool, got int |
| `x is alive;` (where x is int) | Type mismatch in assignment: expected int, got bool |
| `yield 10;` (in a bool function) | Return type mismatch: expected bool, got int |

---

## 6. How They All Connect

### The Complete Data Flow

```
                    "bind x:int is 10;"
                           │
                    ┌──────┴──────┐
                    │    LEXER    │
                    └──────┬──────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
        Token(BIND)  Token(IDENT,"x") Token(INT_LITERAL,"10") ...
              │            │            │
              └────────────┼────────────┘
                           │
                    ┌──────┴──────┐
                    │   PARSER   │
                    └──────┬──────┘
                           │
                     VarDecl Node
                    ┌──────┴──────┐
                    │ name: "x"   │
                    │ type: "int" │
                    │ value: ──────┼──→ LiteralExpr("10")
                    └─────────────┘
                           │
                    ┌──────┴──────┐
                    │  SEMANTIC   │
                    └──────┬──────┘
                           │
              typeOf(LiteralExpr("10")) → "int"
              declared type: "int"
              "int" == "int"  →  ✓ PASS
              declare("x", "int") in scope stack
```

### Module Dependency Graph

```
token.h ◄──── lexer.h/cpp
   │
   └──────── ast.h ◄──── ast_printer.h
               │
               └──── parser.h/cpp
               │
               └──── semantic.h/cpp
                        │
                 main.cpp (orchestrator)
```

Every module only depends on the modules above it in this graph. The Lexer has no
knowledge of the Parser; the Parser has no knowledge of the Semantic Analyzer.
This strict layering means any module can be tested independently and replaced
without breaking the rest of the system.

### CMake Test Integration

```cmake
Test 1: lexer_hello          → validates token stream output
Test 2: parser_hello         → validates AST generation + pipeline completion
Test 3: semantic_hello       → validates type-safe code passes analysis
Test 4: semantic_type_mismatch → validates "bind x:int is alive;" is rejected
Test 5: semantic_undefined   → validates "emit y;" (undeclared) is rejected
```

Running `cmake --workflow --preset ci` executes all 5 tests in one command,
giving you a complete regression suite that verifies every phase of the compiler
pipeline from end to end.

---

## Summary

| Phase | Theory | LiveLy Implementation | Key File |
|-------|--------|----------------------|-----------|
| Lexer | DFA / Finite State Machine | Character-by-character scanner with keyword hash map | `lexer.cpp` (183 lines) |
| Parser | LL(1) Recursive Descent | One function per grammar rule, precedence via call depth | `parser.cpp` (321 lines) |
| AST | Tree data structure | C++17 class hierarchy with `unique_ptr` ownership | `ast.h` (128 lines) |
| Semantic | Symbol Table + Type System | Scope stack with inside-out lookup and recursive type inference | `semantic.cpp` (198 lines) |

Each phase transforms the program into a progressively more refined representation,
catching different classes of errors along the way. Together, they form the complete
**frontend** of the LiveLy compiler — everything up to the point where we start
generating executable code (bytecode, VM, JIT — the backend phases still to come).
