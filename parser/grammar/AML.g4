grammar AML;

// ============================================================
// File kinds
// ============================================================

// Module: interleaved declaration groups and definitions.
// Example:
//   nil/0  | cons/2.
//   append = Y (self => a => b => a b (h => t => cons h (self t b))).
moduleFile
    : moduleItem* EOF
    ;

moduleItem
    : declarationGroup eol
    | definition eol
    ;

// Statement file: lhs ':' rhs pairs.
// Example:
//   multiply 3 4 12 : true.
//   eq (double 2) 4 : is_correct.
statementFile
    : (statement eol)* EOF
    ;

eol
    : '.'
    ;

// ============================================================
// Declarations
// ============================================================

declarationGroup
    : declaration ('|' declaration)*
    ;

declaration
    : NAME '/' NATLIT
    ;

// ============================================================
// Definitions
// ============================================================

definition
    : NAME '=' expr
    ;

// ============================================================
// Statements
// ============================================================

statement
    : expr ':' expr
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

abstraction
    : NAME '=>' expr
    ;

application
    : head arg+
    ;

head
    : '(' expr ')'
    | atom
    ;

arg
    : '(' expr ')'
    | abstraction
    | atom
    ;

atom
    : NAME
    | nat
    | int
    | CHARLIT
    | STRLIT
    | list
    ;

nat
    : ENCODING? NATLIT
    ;

int
    : ENCODING? (POSINTLIT | NEGINTLIT)
    ;

list
    : ENCODING? '[' (expr (',' expr)*)? ']'
    ;

// ============================================================
// Lexer rules
// ============================================================

ENCODING : '<' [a-z]+ '>' ;

NATLIT : [0-9]+ ;

POSINTLIT : '+' [0-9]+ ;

NEGINTLIT : '-' [1-9][0-9]* ;

CHARLIT : '\'' ( ~['\\\r\n] | '\\' . ) '\'' ;

STRLIT : '"' ( ~["\\\r\n] | '\\' . )* '"' ;

NAME : [a-zA-Z_][a-zA-Z0-9_]* ;

LINE_COMMENT  : '//' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '/*' .*? '*/' -> skip ;
WS            : [ \t\r\n]+    -> skip ;
