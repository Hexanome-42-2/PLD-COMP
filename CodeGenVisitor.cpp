#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    visitChildren(ctx);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
	
    //add IR instr de déclaration
    
    // ~~~~~~ CAN'T ASSIGN AND DECLARE ATM ~~~~~~
	// If variable is initialized immediately (e.g., int x = 4;)
	// if (ctx->expr()) {
	// 	// Visit the expression, putting the result in %eax
	// 	visit(ctx->expr());

	// 	// Find the variable in memory
	// 	std::string varName = ctx->VAR()->getText();
	// 	int offset = symbolTable->getVariableOffset(varName); // Get the variable's memory offset

	// 	// Move the value from %eax into the variable's memory space
	// 	std::cout << "    movl %eax, " << offset << "(%rbp)\n";
	// }
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	// Visit the expression, evaluating the right side and putting the result in %eax
	visit(ctx->expr());
	
	std::string varName = ctx->VAR()->getText();
	int offset = symbolTable->getVariableOffset(varName);

	std::cout << "    movl %eax, " << offset << "(%rbp)\n";
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturnStatement(ifccParser::ReturnStatementContext *ctx) {
	// Evaluate the result of the expression
	visit(ctx->expr());

	return 0;
}

// ~~~~~~~~ Expressions ~~~~~~~~

antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
	visit(ctx->expr_unary());
	if (ctx->NEG()) {
    	std::cout << "    negl %eax\n";
	}
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
	
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->VAR()->getText();
	int offset = symbolTable->getVariableOffset(varName);

	std::cout << "    movl " << offset << "(%rbp), %eax\n";
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAdd(ifccParser::AddContext *ctx) { 
	visit(ctx->lExpr);
	const std::string tmpVar = symbolTable->addTemporaryVariable();
    symbolTable->updateMaxOffset();
	std::cout << "    movl %eax, " << symbolTable->getVariableOffset(tmpVar) << "(%rbp)\n"; 

	visit(ctx->rExpr);
	std::cout << "    movl " << symbolTable->getVariableOffset(tmpVar) << "(%rbp), %edx\n";
	std::cout << "    addl %edx, %eax\n";	
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitMult(ifccParser::MultContext *ctx) { 
	visit(ctx->lExpr);	
	const std::string tmpVar = symbolTable->addTemporaryVariable();
	symbolTable->updateMaxOffset();
	std::cout << "    movl %eax, " << symbolTable->getVariableOffset(tmpVar) << "(%rbp)\n";

	visit(ctx->rExpr);
	std::cout << "    movl " << symbolTable->getVariableOffset(tmpVar) << "(%rbp), %edx\n";
	std::cout << "    imull %edx, %eax\n";
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
	visit(ctx->expr());
	return 0;
}
