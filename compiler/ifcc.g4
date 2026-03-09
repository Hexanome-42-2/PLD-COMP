grammar ifcc;

axiom : prog EOF ;

prog		: 'int' 'main' '(' ')' '{' statement* return_statement'}' ;

// 2. Defines what a statement is
statement	: 'int' VAR (',' VAR)* ';'		# DeclareStatement
			| VAR '=' expr ';'				# AssignStatement
			;

// 3. Defines what an expression is
expr		: ( NEG )? expr_unary			# UnaryExpr
			| lExpr=expr '*' rExpr=expr		# Mult
			| lExpr=expr '+' rExpr=expr		# Add
			;
	
expr_unary	: CONST							# ConstExpr
			| VAR							# VarExpr
			| '(' expr ')'					# Par
			;

return_statement	: RETURN expr ';'		# ReturnStatement
					;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

NEG			: '-' ;
RETURN		: 'return' ;
VAR			: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
