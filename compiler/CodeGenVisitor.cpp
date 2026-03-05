#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    #ifdef __APPLE__
    std::cout << ".globl _main\n" ;
    std::cout << " _main: \n" ;
    #else
    std::cout << ".globl main\n" ;
    std::cout << " main: \n" ;
    #endif

    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";
    visitChildren(ctx);
    
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
	// If variable is initialized immediately (e.g., int x = 4;)
	if (ctx->expr()) {
		// Visit the expression, putting the result in %eax
		visit(ctx->expr());

		// Find the variable in memory
		std::string varName = ctx->VAR()->getText();
		int offset = symbolTable[varName].index;

		// Move the value from %eax into the variable's memory space
		std::cout << "    movl %eax, " << offset << "(%rbp)\n";
	}
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	// Visit the expression, evaluating the right side and putting the result in %eax
	visit(ctx->expr());
	
	std::string varName = ctx->VAR()->getText();
	int offset = symbolTable[varName].index;

	std::cout << "    movl %eax, " << offset << "(%rbp)\n";
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturnStatement(ifccParser::ReturnStatementContext *ctx) {
	// Evaluate the result of the expression
	visit(ctx->expr());

	std::cout << "    popq %rbp\n";
	std::cout << "    ret \n";
	return 0;
}

// ~~~~~~~~ Expressions ~~~~~~~~

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
	std::string val = ctx->CONST()->getText();
	std::cout << "    movl $" << val << ", %eax\n";
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->VAR()->getText();
	int offset = symbolTable[varName].index;

	std::cout << "    movl " << offset << "(%rbp), %eax\n";
	return 0;
}
