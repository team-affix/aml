grammar AML;

// ============================================================
// File kinds — three separate top-level grammars
// ============================================================

// Declaration groups: NAME/arity members separated by |, one group per line.
// Example:
//   true/0 | false/0.
//   suc/1 | zero/0.
declarationFile
    : (declarationGroup eol)* EOF
    ;

// Definitions: NAME = expr, one per line.
// Example:
//   not = b => b false true.
definitionFile
    : (definition eol)* EOF
    ;

// Statement file: input : label pairs (one dataset).
// Example:
//   multiply 3 4 12 : true.
//   multiply 3 4 5 : false.
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
    : NAME '/' POSINT
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
    | encodedNat
    | intLit
    | CHARLIT
    | STRLIT
    | encodedList
    ;

encodedNat
    : encoding? NATLIT
    ;

intLit
    : POSINT
    | NEGINTLIT
    ;

encodedList
    : encoding? '[' (expr (',' expr)*)? ']'
    ;

encoding
    : '<' ( CHURCH | SCOTT ) '>'
    ;

// ============================================================
// Lexer rules
// ============================================================

CHURCH : 'church' ;
SCOTT  : 'scott'  ;

NATLIT : [0-9]+ 'N' ;

POSINT : [0-9]+ ;

NEGINTLIT : '-' [1-9][0-9]* ;

CHARLIT : '\'' ( ~['\\\r\n] | '\\' . ) '\'' ;

STRLIT : '"' ( ~["\\\r\n] | '\\' . )* '"' ;

NAME : [a-zA-Z_][a-zA-Z0-9_]* ;

LINE_COMMENT  : '//' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '/*' .*? '*/' -> skip ;
WS            : [ \t\r\n]+    -> skip ;
