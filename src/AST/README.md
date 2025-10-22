# AST

Defines the abstract syntax tree structure used across parsing, semantic analysis, and code generation, plus debugging utilities to inspect it.

## Files

- `ast_node.h` / `ast_node.c`
  - Declares the `ASTNode` union, covering programs, declarations, expressions, statements, preprocessor constructs, and designators.
  - Factory helpers such as `createProgramNode`, `createBlockNode`, `createFunctionDefinitionNode`, `createBinaryExpressionNode`, `createWhileLoopNode`, etc., allocate and initialise fully-typed nodes.
  - Utility routines manage node linking (`appendStatement`), cloning, and common conversions.
- `ast_printer.h` / `ast_printer.c`
  - `printAST(ASTNode* node, int indent)` recursively dumps the tree with indentation, tagging node types and key attributes.
  - Visitors for each node form make it easy to visualise parser output during debugging.

## Interaction

Parser modules construct AST nodes exclusively through this API so downstream passes (`Syntax`, `code_gen`) can rely on consistent layout. Designated initialiser support threads through to `ASTNode` via the `Parser/Helpers/designated_init` types.
