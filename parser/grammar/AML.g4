grammar AML;

// ============================================================
// File kinds — three separate top-level grammars
// ============================================================

// Constructor declarations only: NAME/arity groups separated by |.
// Example:
//   true/0 | false/0
//   suc/1 | zero/0
declarationFile
    : constructorGroup* EOF
    ;

// Function definitions only: NAME = expr.
// Example:
//   not = b => b false true
definitionFile
    : functionDef* EOF
    ;

// Training data: one statement per line, terminated by '.'
// Example:
//   multiply 3 4 12.
//   not false.
trainingFile
    : statement* EOF
    ;

statement
    : expr '.'
    ;

// ============================================================
// Declarations
// ============================================================

constructorGroup
    : constructor ('|' constructor)*
    ;

constructor
    : NAME '/' POSINT
    ;

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
