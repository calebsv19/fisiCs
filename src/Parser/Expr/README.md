Expression Parsing (Pratt + Legacy) — Cheat Sheet
Overview
This group implements expression parsing two ways: a Pratt parser (powerful, precedence-driven, good for postfix chains 
and ternary/assignments) and the legacy recursive-descent chain. A runtime flag on Parser (parser->mode) decides which 
path to use. The Pratt core (nud/led) glues tightly to AST builders, type lookahead, and function/array/member postfix 
parsing. 




		=====  Files & roles  ====
Parser/Expr/parser_expr_pratt.c — Pratt engine: precedence table, nud/led, calls/arrays/member access, cast/sizeof 
helpers. 
parser_expr_pratt
Parser/Expr/parser_expr_pratt.h — Public Pratt API + helpers (precedence/rbp, sizeof/cast/call helpers). 
parser_expr_pratt
Parser/Expr/parser_expr.c — Dispatcher (parseExpression) + full legacy recursive chain (assignment → ternary → 
boolean → … → primary) and statement-level router. Bridges to Pratt when enabled. 
parser_expr
Parser/Expr/parser_expr.h — Public expression API for both modes. 
parser_expr




		Public API (call these from other modules)
ASTNode* parseExpression(Parser* p); — Top entry; uses Pratt or legacy based on p->mode. 
parser_expr
ASTNode* handleExpressionOrAssignment(Parser* p); — Statement-level expression; enforces trailing ;, wraps comma-lists 
as a block. 
parser_expr
Pratt-only:
ASTNode* parseExpressionPratt(Parser* p, int minPrec); — Core Pratt entry with precedence floor. 
parser_expr_pratt
int getTokenPrecedence(TokenType t); int getTokenRightBindingPower(TokenType t); — Precedence / associativity helpers. 
parser_expr_pratt


			Function index (ultra-concise)
Pratt core (parser_expr_pratt.c)
parseExpressionPratt — Pratt loop: nud, then led while precedence > floor. 
parser_expr_pratt
nud (static) — Prefix/literals/grouping; dispatches calls; handles casts/sizeof. 
parser_expr_pratt
led (static) — Infix/postfix: binary, assigns, (), [], ., ->, ?:, ,. 
parser_expr_pratt
getTokenPrecedence — Precedence table; higher binds tighter. 
parser_expr_pratt
getTokenRightBindingPower — Right-assoc ops (assign, ?:) lower RHS floor. 
parser_expr_pratt
ledFunctionCall / parseFunctionCallPratt — Parse arg list; build call node. 
parser_expr_pratt
parseCastExpressionPratt — (type) expr with abstract declarator consumption. 
parser_expr_pratt
parseSizeofExpressionPratt — sizeof (type) vs sizeof expr disambiguation. 
parser_expr_pratt
consumeBalancedParens / consumeAbstractDeclarator — Skip groups/arrays in types. 
parser_expr_pratt
looksLikeParenTypeName — Probe lookahead: (type) form. 
parser_expr_pratt


Legacy chain (parser_expr.c)
parseCommaExpression[Recursive] — Top comma list; builds/expands comma AST. 
parseAssignmentExpression — RHS right-assoc; compounds synthesize a op= b as a = a op b. 
parseTernaryExpression — cond ? A : B using parseExpression for arms. 
parseBooleanExpression — || / &&; optional inline ternary. 
parseComparisonExpression — == != < <= > >=. 
parseBitwiseOr/Xor/And — | ^ & ladders. 
parseBitwiseShift — << >>. 
parseAdditionSubtraction — + -. 
parseMultiplication — * / %. 
parseUnaryOrCast — Distinguish (type) cast vs grouped expr. 
parseFactor — Prefix ++ -- * + - ! ~ and then parsePostfixExpression. 
parseCastExpression — Legacy cast parse. 
parsePostfixExpression — Identifier/primary, then chain . -> [] () and postfix ++/--. 
parsePrimary — literals, sizeof, identifiers, grouped (...). 
parseSizeofExpression — Legacy sizeof handling. 
handleExpressionOrAssignment — Ensures ;, wraps comma as block. 





Call map (who calls what)

Dispatch & top level
parseExpression → parseExpressionPratt (when PARSER_MODE_PRATT) else → legacy path (parseCommaExpression → …). 
parser_expr


Pratt engine
parseExpressionPratt → nud (first) → loop of led. 
parser_expr_pratt
nud
→ literals/identifiers → AST creators
→ identifier + ( → parseFunctionCallPratt
→ ( + type-lookahead → parseCastExpressionPratt
→ prefix ops → parseExpressionPratt with tight floor (11)
→ sizeof → parseSizeofExpressionPratt. 
parser_expr_pratt
led
→ binary/assign → parseExpressionPratt(getTokenRightBindingPower(op))
→ postfix ++/-- → unary node
→ ( → ledFunctionCall
→ [ → parseExpressionPratt(0) then require ]
→ ./-> → consume identifier
→ ?: → parse both arms at right-assoc floor
→ , → flatten into comma node. 
parser_expr_pratt


Legacy chain (left-to-right ladder)
parseAssignmentExpression → parseTernaryExpression → parseBooleanExpression → parseComparisonExpression → 
parseBitwiseOr → parseBitwiseXor → parseBitwiseAnd → parseBitwiseShift → parseAdditionSubtraction → 
parseMultiplication → parseUnaryOrCast → parseFactor → parsePostfixExpression → parsePrima

Each step may loop on 
its operators, building left-assoc binary nodes; assignment recurses right-assoc. 






Include/dep glue (quick view)
Pratt includes: parser_helpers.h, parser_decl.h, parser_array.h, parser_func.h, parser_lookahead.h, parser_expr.h, 
AST/ast_node.h, lexer headers. Uses advance, printParseError, AST creators, and type lookahead utilities (e.g., 
parseType, probes, clones). 
parser_expr_pratt
Legacy includes the same parser/AST/lexer stack and calls into array access and function-call helpers as needed. 
parser_expr
Pratt header exposes only the safe surface (parseExpressionPratt, precedence helpers, and parsing helpers) for other 
parser modules. 
parser_expr_pratt
Expression header exposes the unified API for the rest of the compiler. 
parser_expr




Gotchas / edge cases
Associativity rules: Assignments and ?: are right-assoc in Pratt via rbp = p-1. Everything else is left-assoc. Get 
these floors wrong and parses drift. 
parser_expr_pratt
Postfix chains: Pratt led supports call[idx].field->call++ style chaining; legacy does it in parsePostfixExpression. 
Keep both consistent. 
parser_expr_pratt
 
parser_expr
Cast vs group: Both implementations probe (type) vs (expr); Pratt uses clone/probe + abstract-declarator consumption. 
Mis-detecting casts will nuke precedence. 
parser_expr_pratt
Comma flattening: Pratt flattens comma lists into a single node; legacy builds a list too—avoid double-nesting. 
parser_expr_pratt
 
parser_expr
Statement semicolons: handleExpressionOrAssignment enforces ; and wraps top-level comma as a block—don’t re-emit 
semicolons in callers. 
parser_expr


Minimal usage snippet (conceptual)
Set parser->mode = PARSER_MODE_PRATT; then call parseExpression(p) for general expressions, 
handleExpressionOrAssignment(p) for statement context. The Pratt engine will handle calls, arrays, member access, 
ternary, and assignments in one pass. 
parser_expr

