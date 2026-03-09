grammar ifcc;

axiom : prog EOF ;

// 1. '+' allows one or more statements
prog		: 'int' 'main' '(' ')' '{' statement* return_statement'}' ;

// 2. Defines what a statement is
statement	: 'int' VAR (',' VAR)* ';'	# DeclareStatement
			| VAR '=' expr ';'			# AssignStatement
			;

// 3. Defines what an expression is
expr	: CONST							# ConstExpr
		| VAR							# VarExpr
		| '(' expr ')'					# Par
		| lExpr=expr '*' rExpr=expr		# Mult
		| lExpr=expr '+' rExpr=expr		# Add
		;

return_statement	: RETURN expr ';'	# ReturnStatement
					;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

RETURN		: 'return' ;
VAR			: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
