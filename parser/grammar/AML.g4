grammar AML;

// ============================================================
// Parser rules
// ============================================================

program
    : declaration* EOF
    ;

declaration
    : constructorGroup
    | functionDef
    ;

// A constructor group: one or more constructors separated by |.
// Each group encodes one algebraic type worth of eliminators.
// Example:
//   true/0 | false/0
//   suc/1 | zero/0
constructorGroup
    : constructor ('|' constructor)*
    ;

constructor
    : NAME '/' POSINT
    ;

// Function definitions use curried lambda-calculus style.
// Self-reference is allowed; the transpiler detects it and wraps with Y.
// Example:
//   Y   = f => (x => f (x x)) (x => f (x x))
//   not = b => b false true
functionDef
    : NAME '=' expr
    ;

// ============================================================
// Expressions
// ============================================================

expr
    : abstraction
    | application
    | atom
    | '(' expr ')'
    ;

// Abstraction: always has =>.  Body is a full expr (right-associative).
// a => b => c  parses as  a => (b => c)
abstraction
    : NAME '=>' expr
    ;

// Application: a head applied to one or more args, left-folded.
// f x y z → ((f x) y) z
application
    : head arg+
    ;

// Head (function position): any expr is allowed — use parens for complex ones.
//   f        plain atom (name or literal)
//   (f x)    grouped application
//   (x => x) grouped abstraction
head
    : '(' expr ')'
    | atom
    ;

// Arg (argument position): any expr is allowed — use parens for applications.
//   x            plain atom
//   x => y       bare abstraction (no parens needed; body extends right)
//   (g x)        grouped application
//   (x => y)     grouped abstraction
arg
    : '(' expr ')'
    | abstraction
    | atom
    ;

atom
    : NAME
    | encodedNat
    | intLit
    | CHARLIT
    | STRLIT
    | encodedList
    ;

// Nat literals (N suffix).  Encoding annotation is optional; default is scott.
// Examples: 42N, <church> 42N, <scott> 0N
encodedNat
    : encoding? NATLIT
    ;

// Integer literals (no N suffix).
// Positive: 42  →  pos (binary nat)
// Negative: -12 →  negsuc (binary of 11)
// -0 is rejected at lex time (NEGINTLIT only matches non-zero negatives).
intLit
    : POSINT
    | NEGINTLIT
    ;

// List literals.  Encoding annotation is optional; default is scott (cons/nil).
// Example: [a, b, c], <church> [1N, 2N]
encodedList
    : encoding? '[' (expr (',' expr)*)? ']'
    ;

// Encoding annotation: only valid on nat literals and list literals.
encoding
    : '<' ( CHURCH | SCOTT ) '>'
    ;

// ============================================================
// Lexer rules — keywords before NAME so they take priority
// ============================================================

CHURCH : 'church' ;
SCOTT  : 'scott'  ;

// Nat literal: digits immediately followed by N (no whitespace).
NATLIT : [0-9]+ 'N' ;

// Non-negative integer (also used for constructor arities).
POSINT : [0-9]+ ;

// Negative integer: must start with a non-zero digit after '-' so that
// -0 produces a lex error (no token matches), failing at parse time.
NEGINTLIT : '-' [1-9][0-9]* ;

// Character literal: single character, supports backslash escapes.
CHARLIT : '\'' ( ~['\\\r\n] | '\\' . ) '\'' ;

// String literal: zero or more characters, supports backslash escapes.
STRLIT : '"' ( ~["\\\r\n] | '\\' . )* '"' ;

NAME : [a-zA-Z_][a-zA-Z0-9_]* ;

LINE_COMMENT  : '//' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '/*' .*? '*/' -> skip ;
WS            : [ \t\r\n]+    -> skip ;
