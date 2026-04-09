# Next Task: Semantic Analysis (Phase 5)

> Previous task (Parser Integration + AST Printer) has been completed and verified.
> Both `lexer_hello` and `parser_hello` CTest tests pass.

---

## What Was Just Completed

- **Parser hooked into `main.cpp`** — the full pipeline now runs: Source → Lexer → Parser → AST Print.
- **AST Printer created** (`src/ast/ast_printer.h`) — produces a visual tree in the terminal showing every node, its children, and correct operator precedence.
- **CTest updated** — `parser_hello` test validates that the AST is generated and the pipeline completes without errors.
- **All 7 top-level statements** from `hello.lv` parse correctly: 3 variable declarations, 1 assignment, 1 if/else, 1 loop, 1 function declaration.

---

## Next Task: Build the Semantic Analyzer

### Why This Is Next

The Parser currently accepts **syntactically valid** code, but it cannot catch **logically invalid** code. For example:

```lively
bind x:int is alive;     ← assigning a bool to an int (type mismatch!)
bind x:int is 10;
bind x:int is 20;        ← redeclaring x in the same scope (duplicate!)
emit y;                  ← y was never declared (undefined variable!)
```

All of these would silently parse into an AST today. The Semantic Analyzer will catch them before bytecode generation.

### Scope of Work

#### A. Symbol Table (`src/semantic/symbol_table.h`)

A data structure to track declared variables and their types within scopes.

| Feature | Detail |
|---------|--------|
| Scope Stack | `std::vector<std::unordered_map<string, string>>` — push on function/block entry, pop on exit |
| `declare(name, type)` | Register a variable; error if already declared in current scope |
| `lookup(name)` → type | Walk the scope stack inside-out; error if not found |
| `enterScope()` / `exitScope()` | Push/pop the scope stack |

#### B. Semantic Analyzer (`src/semantic/semantic.h` + `semantic.cpp`)

Walk the AST and enforce rules:

| Rule | Check |
|------|-------|
| **Type Safety** | `bind x:int is <expr>` — the expression must evaluate to `int` |
| **No Redeclaration** | Two `bind x:int` in the same scope → error |
| **Variable Exists** | Using `x` in an expression → `x` must be declared |
| **Function Params** | Parameter types must match `int` or `bool` |
| **Return Types** | `yield` expression type must match function's declared return type |
| **Condition Types** | `check(...)` and `cycle(...)` conditions must evaluate to `bool` |

#### C. Integration Into the Pipeline

Update `main.cpp` to add a Phase 4 step:

```cpp
#include "semantic/semantic.h"

// After parsing...
SemanticAnalyzer analyzer;
analyzer.analyze(ast);
std::cout << "[Phase 4] Semantic analysis passed.\n";
```

#### D. Testing

1. **Positive test:** `hello.lv` should pass semantic analysis cleanly.
2. **Negative tests:** Create `examples/errors/` directory with intentionally broken programs:
   - `type_mismatch.lv` — `bind x:int is alive;`
   - `undeclared.lv` — `emit y;`
   - `redeclare.lv` — double `bind x:int`
3. **CTest entries:** Each negative test should `FAIL_REGULAR_EXPRESSION` with the expected error message.

#### E. Expression Type Inference

To check types, we need a `typeOf(Expression*)` function that evaluates the type of an AST expression:

| Expression | Inferred Type |
|------------|---------------|
| `LiteralExpr` with digits | `int` |
| `LiteralExpr` with `alive`/`dead` | `bool` |
| `VariableExpr` | looked up from symbol table |
| `BinaryExpr` with `+`,`-`,`*`,`/` | both operands must be `int`, result is `int` |
| `BinaryExpr` with `>`,`<`,`>=`,`<=`,`==`,`!=` | both operands must be `int`, result is `bool` |

---

## Files to Create / Modify

| Action | File | Purpose |
|--------|------|---------|
| **CREATE** | `src/semantic/symbol_table.h` | Scope stack for variable tracking |
| **CREATE** | `src/semantic/semantic.h` | Analyzer class declaration |
| **CREATE** | `src/semantic/semantic.cpp` | Analyzer implementation |
| **MODIFY** | `src/main.cpp` | Add Phase 4 semantic analysis call |
| **MODIFY** | `CMakeLists.txt` | Add `semantic_hello` test |
| **CREATE** | `examples/errors/type_mismatch.lv` | Negative test case |
| **CREATE** | `examples/errors/undeclared.lv` | Negative test case |
| **CREATE** | `examples/errors/redeclare.lv` | Negative test case |

---

## Success Criteria

- `cmake --workflow --preset ci` passes all tests (lexer, parser, semantic — positive and negative).
- `hello.lv` produces clean terminal output through all 4 phases.
- Intentionally broken programs produce clear error messages with line numbers.
- No changes to `lexer/` or `parser/` modules (strict backward compatibility).
