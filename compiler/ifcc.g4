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
statement	: TYPE NAME '=' expr ';'			# DeclareAssignStatement
			| TYPE NAME (',' NAME)* ';'			# DeclareStatement
			| NAME '=' expr ';'				    # AssignStatement
			| NAME '(' argument? ')' ';'		# FunctionCallStatement
			| RETURN expr? ';'                  # ReturnStatement
			;

// 4. Defines what an expression is
expr		: ( NEG )? expr_unary			    # UnaryExpr
			| lExpr=expr MULTOP rExpr=expr		# MultDiv
			| lExpr=expr ADDOP rExpr=expr		# AddSub
			;

expr_unary	: CONST							    # ConstExpr
			| NAME '(' argument? ')'			# FuncCall
			| NAME							    # VarExpr
			| '(' expr ')'					    # Par
			;

expr_bool   : expr                              # Bool
            | expr COMPOP expr                  # Comp
            | expr EQOP expr                    # EQ
            ;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

NEG			: '-' | '!' ;
ADDOP       : '+' | '-' ;
MULTOP      : '*' | '/' | '%' ;
BITOP       : '&' | '|' | '^' ;
COMPOP      : '<' | '>' ;
EQOP        : '==' | '!=' ;
RETURN		: 'return' ;
TYPE		: 'int' | 'void' ;
NAME		: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
LINE_COMMENT: '//' ~[\r\n]* -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
