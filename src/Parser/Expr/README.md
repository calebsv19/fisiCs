# Expression Parsing

Two expression engines live side by side so the compiler can evolve without losing the legacy recursive ladder.

## Files

- `parser_expr.h` / `parser_expr.c`
  - Public façade: `parseExpression(Parser*)` selects Pratt vs legacy based on `parser->mode`.
  - Statement helpers: `handleExpressionOrAssignment()` enforces trailing semicolons when expressions act as stand-alone statements.
  - Legacy recursive-descent chain from `parseCommaExpression()` down to `parsePrimary()`. Each rung corresponds to one precedence level (assignment, ternary, logical, bitwise, additive, multiplicative, prefix/postfix, literals).
- `parser_expr_pratt.h` / `parser_expr_pratt.c`
  - Pratt implementation (`parseExpressionPratt(Parser*, int minPrec)`) with per-token `nud` (prefix/primary) and `led` (infix/postfix) handlers.
  - Precedence utilities: `getTokenPrecedence()` and `getTokenRightBindingPower()` encode binding and associativity.
  - Coordinator file for the Pratt core loop, `nud`/`led` dispatch, grouped-expression handling, and function-call parsing.
- `parser_expr_pratt_type_forms.c`
  - Type-directed Pratt helpers extracted from the coordinator: cast parsing, compound literals, `sizeof`, `alignof`, and the parenthesized type-probe helpers (`consumeBalancedParens`, `looksLikeParenTypeName`, abstract-declarator capture).
  - Uses `parseTypeCtx(..., TYPECTX_Strict)` when probing casts, `sizeof`, and compound literals so expression parsing doesn’t misclassify `(identifier)` unless the identifier is a known type.

## External API

```c
ASTNode* parseExpression(Parser* p);
ASTNode* handleExpressionOrAssignment(Parser* p);

// Pratt-only when PARSER_MODE_PRATT
ASTNode* parseExpressionPratt(Parser* p, int minPrec);
int getTokenPrecedence(TokenType t);
int getTokenRightBindingPower(TokenType t);
```

## Pratt flow at a glance

1. `parseExpressionPratt(p, floor)` calls `nud()` on the current token to parse a prefix/literal/group.
2. While the next token’s precedence is higher than `floor`, `led()` consumes it and recurses with the right-binding power.
3. Postfix operators (`()`, `[]`, `.`, `->`, `++`, `--`) are handled inside `led()`, enabling fluent chains like `foo()->bar[i]++`.

Legacy mode mirrors standard C precedence; having both systems makes it easy to compare outputs during migrations.
