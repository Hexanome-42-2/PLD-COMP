grammar ifcc;

axiom : prog EOF ;

// 1. '+' allows one or more statements
prog		: 'int' 'main' '(' ')' '{' statement+ '}' ;

// 2. Defines what a satatement is
statement	: 'int' VAR ( '=' expr )? ';'	# DeclareStatement
		| VAR '=' expr ';'		# AssignStatement
		| RETURN expr ';'		# ReturnStatement
		;

// 3. Defines what an expression is
expr		: CONST		# ConstExpr
		| VAR		# VarExpr
		;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

RETURN		: 'return' ;
VAR		: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
