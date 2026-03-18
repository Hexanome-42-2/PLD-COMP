#include "StaticAnalysisVisitor.h"

std::any StaticAnalysisVisitor::visitProg(ifccParser::ProgContext *ctx) {
	// Pass 1: Register all function signatures before analyzing bodies
	for (auto* funcCtx : ctx->fonction()) {
		auto* f = dynamic_cast<ifccParser::FunctionContext*>(funcCtx);
		if (f) {
			std::string name = f->funcName->getText();
			std::string retType = f->INT_TYPE() ? "int" : "void";
			int paramCount = 0;
			if (f->parameters()) {
				auto* params = dynamic_cast<ifccParser::ParamListContext*>(f->parameters());
				if (params) {
					paramCount = params->VAR().size();
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

	// Register main function
	functionSignatures["main"] = {"int", 0};

	// Pass 2: Visit all children for semantic analysis
	visitChildren(ctx);

	// Check Rule: "Une variable declaree est utilisee au moins une fois."
	std::vector<std::string> unusedVars = symbolTable->getUnusedVariables();
	for (const std::string& varName : unusedVars) {
		std::cerr << "Warning: Variable '" << varName << "' declared but not used." << std::endl;
	}

	// Initialize temporary variables' offset in symbol table
	symbolTable->InitializeTmpOffset();
	return 0;
}

std::any StaticAnalysisVisitor::visitFunction(ifccParser::FunctionContext *ctx) {
	// Add function parameters to symbol table so body can use them
	if (ctx->parameters()) {
		auto* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters());
		if (params) {
			for (auto* varNode : params->VAR()) {
				std::string paramName = varNode->getText();
				if (symbolTable->getVariable(paramName) == nullptr) {
					symbolTable->addVariable(paramName);
					symbolTable->MarkUsed(paramName); // params are considered "used"
				}
			}
		}
	}

	// Visit the function body (statements, return, etc.)
	visitChildren(ctx);
	return 0;
}

std::any StaticAnalysisVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
	for ( auto Node : ctx->VAR() ) {
		std::string varName = Node->getText();

		if (symbolTable->getVariable(varName) != nullptr) {
			std::cerr << "Error: Variable '" << varName << "' has already been declared." << std::endl;
			hasError = true;
		} else {
			symbolTable->addVariable(varName);
		}
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitDeclareAssignStatement(ifccParser::DeclareAssignStatementContext *ctx) {
	std::string varName = ctx->VAR()->getText();

	if (symbolTable->getVariable(varName) != nullptr) {
		std::cerr << "Error: Variable '" << varName << "' has already been declared." << std::endl;
		hasError = true;
	} else {
		symbolTable->addVariable(varName);
	}

	// Visit the expression on the right side of '='
	visit(ctx->expr());
	return 0;
}

std::any StaticAnalysisVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	std::string varName = ctx->VAR()->getText();

	// Check Rule: "Une variable utilisee dans une expression a ete declaree"
	if (symbolTable->getVariable(varName) == nullptr) {
		std::cerr << "Error: Variable '" << varName << "' assigned before declaration." << std::endl;
		hasError = true;
	}

	// Visit the right side of the equals sign
	visit(ctx->expr());
	return 0;
}

std::any StaticAnalysisVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->VAR()->getText();

	// Check Rule: "Une variable utilisee dans une expression a ete declaree"
	if (symbolTable->getVariable(varName) == nullptr) {
		std::cerr << "Error: Variable '" << varName << "' used in expression before declaration." << std::endl;
		hasError = true;
	} else {
		// Mark as used because it's being read on the right side of an expression!
		symbolTable->MarkUsed(varName);
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitFuncCallExpr(ifccParser::FuncCallExprContext *ctx) {
	std::string funcName = ctx->VAR()->getText();

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

std::any StaticAnalysisVisitor::visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) {
	std::string funcName = ctx->VAR()->getText();

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
