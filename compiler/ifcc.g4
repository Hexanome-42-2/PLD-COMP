grammar ifcc;

axiom : prog EOF ;

prog 		: fonction+ ;

fonction  	: functype=TYPE funcName=NAME '(' parameters? ')' block		# Function
			;

block		: '{' statement* '}'				# BlockStatement
			;

// 1. Defines function parameters (each param has explicit type: int a, int b)
parameters	: TYPE NAME (',' TYPE NAME)*		# ParamList
			;

// 2. Defines function call arguments
argument	: expr (',' expr)*					# ArgumentList
			;

// 3. Defines what a statement is
statement	: TYPE assignStatement (',' assignStatement)* ';'	# DeclareStatement
			| NAME '(' argument? ')' ';'		                # FunctionCallStatement
			| RETURN expr? ';'                                  # ReturnStatement
			| 'if' '(' expr ')' ( ifst=statement | ifbl=block )
			  ('else' ( elst=statement | elbl=block ))?         # IfStatement
			| 'while' '(' expr ')'
			  ( whst=statement | whbl=block )                   # WhileStatement
			| expr ';'								            # ExprStatement
			;

assignStatement : NAME ('=' expr)? ;

// 4. Defines what an expression is
expr		: lExpr=expr MULTOP rExpr=expr		    # MultDiv
			| lExpr=expr op=('-'|'+') rExpr=expr    # AddSub
			| ( op=('-'|'+'|'!') )? expr_unary      # UnaryExpr
			| lExpr=expr BITOP rExpr=expr		    # BitWise
            | lExpr=expr COMPOP rExpr=expr          # Comp
            | lExpr=expr EQOP rExpr=expr            # EQ
            | assignStatement                       # AssignExpr
            ;

expr_unary	: CONST							    # ConstExpr
			| NAME '(' argument? ')'			# FuncCall
			| NAME							    # VarExpr
			| '(' expr ')'					    # Par
			;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

MULTOP      : '*' | '/' | '%' ;
BITOP       : '&' | '|' | '^' ;
COMPOP      : '<' | '>' ;
EQOP        : '==' | '!=' ;
RETURN		: 'return' ;
TYPE		: 'int' | 'void' ;
NAME		: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: ('/*' .*? '*/' | '//' .*? '\n') -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
