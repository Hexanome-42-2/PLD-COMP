#include "StaticAnalysisVisitor.h"

std::any StaticAnalysisVisitor::visitProg(ifccParser::ProgContext *ctx) {
	// 1. Visit all statements in the program
	visitChildren(ctx);

	// 2. Check Rule: "Une variable déclarée est utilisée au moins une fois."
	for (const auto& pair : symbolTable) {
		if (!pair.second.used) {
			std::cerr << "Warning: Variable '" << pair.first << "' declared but not used." << std::endl;
		}
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
	std::string varName = ctx->VAR()->getText();

	if (symbolTable.find(varName) != symbolTable.end()) {
		std::cerr << "Error: Variable '" << varName << "' has already been declared." << std::endl;
		hasError = true;
	} else {
		currIndex -= 4;
		symbolTable[varName] = {currIndex, false};
	}
	
	// If it's initialized at the same line (e.g., int x = 4;)
	if (ctx->expr()) {
		visit(ctx->expr());
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	std::string varName = ctx->VAR()->getText();

	// Check Rule: "Une variable utilisée dans une expression a été déclarée"
	if (symbolTable.find(varName) == symbolTable.end()) {
		std::cerr << "Error: Variable '" << varName << "' assigned before declaration." << std::endl;
		hasError = true;
	}

	// Visit the right side of the equals sign
	visit(ctx->expr());
	return 0;
}

std::any StaticAnalysisVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->VAR()->getText();

	// Check Rule: "Une variable utilisée dans une expression a été déclarée"
	if (symbolTable.find(varName) == symbolTable.end()) {
		std::cerr << "Error: Variable '" << varName << "' used in expression before declaration." << std::endl;
		hasError = true;
	} else {
		// Mark as used because it's being read on the right side of an expression!
		symbolTable[varName].used = true;
	}
	return 0;
}
