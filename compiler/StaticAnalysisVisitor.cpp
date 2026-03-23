#include "StaticAnalysisVisitor.h"

std::any StaticAnalysisVisitor::visitProg(ifccParser::ProgContext *ctx) {
	currSymbolTable = programSymbolTable; // Johny: start with global scope

	// === Pass 1 (our contribution): Register all function signatures before analyzing bodies ===
	for (auto* funcCtx : ctx->fonction()) {
		auto* f = dynamic_cast<ifccParser::FunctionContext*>(funcCtx);
		if (f) {
			std::string name = f->funcName->getText();
			std::string retType = f->functype->getText(); // "int" or "void"
			int paramCount = 0;
			if (f->parameters()) {
				auto* params = dynamic_cast<ifccParser::ParamListContext*>(f->parameters());
				if (params) {
					paramCount = params->NAME().size();
				}
			}

			if (functionSignatures.count(name)) {
				std::cerr << "Error: Function '" << name << "' already defined." << std::endl;
				hasError = true;
			} else {
				functionSignatures[name] = {retType, paramCount};
			}
		}
	}

	// === Pass 2: Visit all children for semantic analysis ===
	visitChildren(ctx);

	// Johny: check for unused variables in the global scope
	std::vector<std::string> unusedVars = programSymbolTable->getUnusedVariables();
	for (const std::string& varName : unusedVars) {
		std::cerr << "Warning: Variable '" << varName << "' declared but not used." << std::endl;
	}

	// Johny: check for unused variables in each function scope
	for (const auto& pair : *functionSymbolTables) {
		std::vector<std::string> unusedFuncVars = pair.second->getUnusedVariables();
		for (const std::string& varName : unusedFuncVars) {
			std::cerr << "Warning: Variable '" << varName << "' declared in function '" << pair.first << "' but not used." << std::endl;
		}
	}

	// Johny: initialize temporary variable offsets
	programSymbolTable->InitializeTmpOffset();
	for (const auto& pair : *functionSymbolTables) {
		pair.second->InitializeTmpOffset();
	}

	return 0;
}

std::any StaticAnalysisVisitor::visitFunction(ifccParser::FunctionContext *ctx) {
	std::string functionName = ctx->funcName->getText();

	// Johny: check for duplicate function names
	if (functionSymbolTables->find(functionName) != functionSymbolTables->end()) {
		std::cerr << "Error: Function '" << functionName << "' has already been declared." << std::endl;
		hasError = true;
		return 0;
	}

	// Johny: create per-function symbol table and switch scope
	SymbolTable* functionTable = new SymbolTable();
	(*functionSymbolTables)[functionName] = functionTable;
	SymbolTable* oldSymbolTable = currSymbolTable;
	currSymbolTable = functionTable;

	// Our contribution: add function parameters to the function's symbol table
	if (ctx->parameters()) {
		auto* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters());
		if (params) {
			for (auto* varNode : params->NAME()) {
				std::string paramName = varNode->getText();
				if (currSymbolTable->getVariable(paramName) == nullptr) {
					currSymbolTable->addVariable(paramName);
					currSymbolTable->MarkUsed(paramName); // params are considered "used"
				}
			}
		}
	}

	// Johny: visit the function block to populate symbol table and check errors
	visit(ctx->block());

	// Johny: restore the previous symbol table
	currSymbolTable = oldSymbolTable;
	return 0;
}

std::any StaticAnalysisVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
	for (auto Node : ctx->NAME()) {
		std::string varName = Node->getText();

		if (currSymbolTable->getVariable(varName) != nullptr) {
			std::cerr << "Error: Variable '" << varName << "' has already been declared." << std::endl;
			hasError = true;
		} else {
			currSymbolTable->addVariable(varName);
		}
	}
	return 0;
}

// Our contribution: int x = expr;
std::any StaticAnalysisVisitor::visitDeclareAssignStatement(ifccParser::DeclareAssignStatementContext *ctx) {
	std::string varName = ctx->NAME()->getText();

	if (currSymbolTable->getVariable(varName) != nullptr) {
		std::cerr << "Error: Variable '" << varName << "' has already been declared." << std::endl;
		hasError = true;
	} else {
		currSymbolTable->addVariable(varName);
	}

	// Visit the expression on the right side of '='
	visit(ctx->expr());
	return 0;
}

std::any StaticAnalysisVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	std::string varName = ctx->NAME()->getText();

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

	if (currSymbolTable->getVariable(varName) == nullptr) {
		std::cerr << "Error: Variable '" << varName << "' used in expression before declaration." << std::endl;
		hasError = true;
	} else {
		currSymbolTable->MarkUsed(varName);
	}
	return 0;
}

// Our contribution: function call in expression context (e.g. return add(1,2); or int x = add(1,2);)
std::any StaticAnalysisVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
	std::string funcName = ctx->NAME()->getText();

	// Check: function exists
	auto it = functionSignatures.find(funcName);
	if (it == functionSignatures.end()) {
		std::cerr << "Error: Function '" << funcName << "' is not defined." << std::endl;
		hasError = true;
		return 0;
	}

	// Check: argument count matches parameter count
	int expectedArgs = it->second.paramCount;
	int actualArgs = 0;
	if (ctx->argument()) {
		auto* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			actualArgs = args->expr().size();
		}
	}

	if (actualArgs != expectedArgs) {
		std::cerr << "Error: Function '" << funcName << "' expects "
				  << expectedArgs << " argument(s), but " << actualArgs
				  << " were provided." << std::endl;
		hasError = true;
	}

	// Check: void function cannot be used in an expression
	if (it->second.returnType == "void") {
		std::cerr << "Error: Void function '" << funcName
				  << "' cannot be used in an expression." << std::endl;
		hasError = true;
	}

	// Visit arguments to check for undeclared variables etc.
	if (ctx->argument()) {
		visit(ctx->argument());
	}

	return 0;
}

// Our contribution: standalone function call (e.g. donothing();)
std::any StaticAnalysisVisitor::visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) {
	std::string funcName = ctx->NAME()->getText();

	// Check: function exists
	auto it = functionSignatures.find(funcName);
	if (it == functionSignatures.end()) {
		std::cerr << "Error: Function '" << funcName << "' is not defined." << std::endl;
		hasError = true;
		return 0;
	}

	// Check: argument count matches parameter count
	int expectedArgs = it->second.paramCount;
	int actualArgs = 0;
	if (ctx->argument()) {
		auto* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			actualArgs = args->expr().size();
		}
	}

	if (actualArgs != expectedArgs) {
		std::cerr << "Error: Function '" << funcName << "' expects "
				  << expectedArgs << " argument(s), but " << actualArgs
				  << " were provided." << std::endl;
		hasError = true;
	}

	// Note: void function as standalone statement is perfectly valid (no error here)

	// Visit arguments to check for undeclared variables etc.
	if (ctx->argument()) {
		visit(ctx->argument());
	}

	return 0;
}
