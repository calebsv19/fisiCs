# C99 Feature Matrix (Final Suite)

Status legend:
- supported
- partial
- unsupported
- planned

## Translation Phases
- preprocessing and macro expansion: supported
- #line directives and location mapping: supported
- trigraphs/digraphs: partial

## Lexical
- integer and float literals: supported
- hex floats: supported
- character escapes and multichar: partial
- universal character names: partial

## Preprocessor
- object/function-like macros: supported
- stringizing and token paste: supported
- variadic macros: supported
- include guards: supported
- macro recursion guard: planned

## Declarations
- complex declarators: supported
- designated initializers: supported
- bit-fields: supported
- flexible arrays: supported
- VLAs: partial

## Expressions
- full precedence and associativity: supported
- conditional operator: supported
- comma operator: supported

## Semantics
- lvalue/rvalue rules: supported
- usual arithmetic conversions: supported
- pointer arithmetic: supported

## Initializers
- aggregate and scalar init: supported
- static constexpr init: partial

## Statements
- if/else/switch/for/goto: supported
- break/continue legality: supported
## Codegen
- basic arithmetic/control flow: supported
- short-circuit: supported
- switch lowering: supported

## Runtime Surface
- stdbool.h, stdint.h, stddef.h, limits.h: supported

## Diagnostics
- recovery and resync: partial
