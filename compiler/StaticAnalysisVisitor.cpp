#include "StaticAnalysisVisitor.h"

std::any StaticAnalysisVisitor::visitProg(ifccParser::ProgContext *ctx) {
	currSymbolTable = programSymbolTable; // Start with the global symbol table
	// 1. Visit all statements in the program
	visitChildren(ctx);

	// 2. Check for unused variables in the global scope
	std::vector<std::string> unusedVars = programSymbolTable->getUnusedVariables();
	for (const std::string& varName : unusedVars) {
		std::cerr << "Warning: Variable '" << varName << "' declared but not used." << std::endl;
	}
	
	// 3. Iterating over all function symbol tables to check for unused variables in each function
	for (const std::pair<std::string, SymbolTable*>& pair : *functionSymbolTables) {
		std::vector<std::string> unusedFuncVars = pair.second->getUnusedVariables();
		for (const std::string& varName : unusedFuncVars) {
			std::cerr << "Warning: Variable '" << varName << "' declared in function '" << pair.first << "' but not used." << std::endl;
		}
	}

	// 4. Initialize temporary variables' offset in symbol table
	programSymbolTable->InitializeTmpOffset();

	for (const std::pair<std::string, SymbolTable*>& pair : *functionSymbolTables) {
		pair.second->InitializeTmpOffset();
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitFunction(ifccParser::FunctionContext *ctx) {
	// 1. Create a new symbol table for this function and add it to the map
	std::string functionName = ctx->funcName->getText();
	SymbolTable* functionTable = new SymbolTable();
	(*functionSymbolTables)[functionName] = functionTable;
	SymbolTable* oldSymbolTable = currSymbolTable; // Save the current symbol table to restore it later
	currSymbolTable = functionTable; // Set the current symbol table to the new function's symbol table
	// TODO : Handle function parameters and add them to the function's symbol table

	// TODO : Handle conflicting function names (e.g., if a function is declared with the same name as a variable in the global scope, or if two functions have the same name)

	// 2. Visit the function block to populate the symbol table and check for errors
	visit(ctx->block());
	currSymbolTable = oldSymbolTable; // Restore the previous symbol table
	return 0;
}

std::any StaticAnalysisVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
	for ( auto Node : ctx->NAME() ) {
		std::string varName = Node->getText();

		if (currSymbolTable->getVariable(varName) != nullptr) {
			std::cerr << "Error: Variable '" << varName << "' has already been declared." << std::endl;
			hasError = true;
		} else {
			currSymbolTable->addVariable(varName);
		}
		// ~~~~~~~ CAN'T ASSIGN AND DECLARE ATM ~~~~~~~~
		// // If it's initialized at the same line (e.g., int x = 4;)
		// if (->expr()) {
		// 	visit(ctx->expr());
		// }
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	std::string varName = ctx->NAME()->getText();

	// Check Rule: "Une variable utilisée dans une expression a été déclarée"
	if (currSymbolTable->getVariable(varName) == nullptr) {
		std::cerr << "Error: Variable '" << varName << "' assigned before declaration." << std::endl;
		hasError = true;
	}

	// Visit the right side of the equals sign
	visit(ctx->expr());
	return 0;
}

std::any StaticAnalysisVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->NAME()->getText();

	// Check Rule: "Une variable utilisée dans une expression a été déclarée"
	if (currSymbolTable->getVariable(varName) == nullptr) {
		std::cerr << "Error: Variable '" << varName << "' used in expression before declaration." << std::endl;
		hasError = true;
	} else {
		// Mark as used because it's being read on the right side of an expression!
		currSymbolTable->MarkUsed(varName);
	}
	return 0;
}
