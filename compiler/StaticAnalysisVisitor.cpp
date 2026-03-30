#include "StaticAnalysisVisitor.h"

std::any StaticAnalysisVisitor::visitIncludeStatement(ifccParser::IncludeStatementContext *ctx) {
	std::string filePath = ctx->file->getText();

	// Append standard include paths to search for the file
	// Check if the file exists (for now only in standard include paths or current directory)
	// --> Later implementations should handle include paths as well
	std::string standardIncludes[] = {"/usr/include/", "/usr/local/include/", "./", "/usr/include/x86_64-linux-gnu/"}; // Add more paths as needed

	bool found = false;
	for (const std::string& includePath : standardIncludes) {
		std::ifstream file(includePath + filePath);
		if (file.good()) {
			filePath = includePath + filePath; // Update filePath to the full path
			found = true;
			break;
		}
	}
	if (!found) {
		std::cerr << "Error: Included file '" << filePath << "' cannot be found in standard include paths." << std::endl;
		hasError = true;
		return 0;
	} else {
		// Create a new parser for the included file and perform static analysis on it
		ANTLRInputStream input(filePath);
		ifccLexer lexer(&input);
		CommonTokenStream tokens(&lexer);
		tokens.fill();
		ifccParser parser(&tokens);
		tree::ParseTree* tree = parser.axiom();
		if (parser.getNumberOfSyntaxErrors() != 0) {
			std::cerr << "Error: Syntax error in included file '" << filePath << "'." << std::endl;
			hasError = true;
		} else {
			bool oldVisitingInclude = isVisitingInclude; // Save the old state of the flag
			isVisitingInclude = true; // Set flag to indicate we're analyzing an included file
			visit(tree); // Recursively analyze the included file
			isVisitingInclude = oldVisitingInclude; // Restore the old state of the flag
		}
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitProg(ifccParser::ProgContext *ctx) {
	currSymbolTable = programSymbolTable;

	// === Pass 1: Register all function signatures before analyzing bodies ===
	for (ifccParser::FonctionContext* funcCtx : ctx->fonction()) {
		ifccParser::FunctionContext* f = dynamic_cast<ifccParser::FunctionContext*>(funcCtx);
		if (f) {
			std::string name = f->funcName->getText();
			std::string retType = f->functype->getText();
            Type retTypeEnum = stringToType(retType);
			// In C, f() means unspecified parameter list (not strictly zero parameters).
			// We encode unknown arity as -1 and skip strict arg-count checks for those functions.
			int paramCount = -1;
			if (f->parameters()) {
				ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(f->parameters());
				if (params) {
					paramCount = params->NAME().size();
				}
			}

			if (functionSignatures.count(name)) {
				std::cerr << "Error: Function '" << name << "' already defined." << std::endl;
				hasError = true;
			} else {
				functionSignatures[name] = {retTypeEnum, paramCount, isVisitingInclude}; // Mark as external if we're currently visiting an include
			}
		}
	}

	// === Pass 2: Visit all children for semantic analysis ===
	visitChildren(ctx);

	std::vector<std::string> unusedVars = programSymbolTable->getUnusedVariables();
	for (const std::string& varName : unusedVars) {
		std::cerr << "Warning: Variable '" << varName << "' declared but might not be used." << std::endl;
	}

	for (const auto& pair : *allSymbolTables) {
		std::vector<std::string> unusedFuncVars = pair.second->getUnusedVariables();
		for (const std::string& varName : unusedFuncVars) {
			std::cerr << "Warning: Variable '" << varName << "' declared in function '" << pair.first << "' but might not be used." << std::endl;
		}
	}

	programSymbolTable->InitializeTmpOffset();
	for (const auto& pair : *allSymbolTables) {
		pair.second->InitializeTmpOffset();
	}

	return 0;
}

std::any StaticAnalysisVisitor:visitFunctionDeclaration(ifccParser::FunctionDeclarationContext *ctx) {
	std::string functionName = ctx->funcName->getText();

	if (functionSymbolTables->find(functionName) != functionSymbolTables->end()) {
		std::cerr << "Error: Function '" << functionName << "' has already been declared." << std::endl;
		hasError = true;
		return 0;
	}

	SymbolTable* functionTable = new SymbolTable();
	(*functionSymbolTables)[functionName] = functionTable;

	if (ctx->parameters()) {
		ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters);
		if (params) {
			for (antlr4::tree::TerminalNode* varNode : params->NAME()) {
				std::string paramName = varNode->getText();
				if (functionTable->getVariable(paramName) == nullptr) {
					functionTable->addVariable(paramName);
					functionTable->MarkUsed(paramName); // params are considered "used"
				}
			}
		}
	}

	return 0;
}

std::any StaticAnalysisVisitor::visitFunctionDefinition(ifccParser::FunctionDefinitionContext *ctx) {
	std::string functionName = ctx->funcName->getText();

	if (allSymbolTables->find(functionName) != allSymbolTables->end()) {
		std::cerr << "Error: Function '" << functionName << "' has already been declared." << std::endl;
		hasError = true;
		return 0;
	}

	SymbolTable* functionTable = new SymbolTable();
	(*allSymbolTables)[functionName] = functionTable;
	SymbolTable* oldSymbolTable = currSymbolTable;

	currSymbolTable = functionTable;
	currIndex = 0; // Reset block index for new function
	currentFunctionName = functionName;

	if (ctx->parameters()) {
		ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters());
		if (params) {
			for (antlr4::tree::TerminalNode* varNode : params->NAME()) {
				std::string paramName = varNode->getText();
				functionTable->addVariable(paramName);
			}
		}
	}

    // Visit the children of the block to not recreate the symbol table for the function body
	visitChildren(ctx->block());

	currSymbolTable = oldSymbolTable;
	return 0;
}

std::any StaticAnalysisVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    SymbolTable* blockTable = new SymbolTable(currSymbolTable);
    SymbolTable* oldSymbolTable = currSymbolTable;
    currSymbolTable = blockTable;

    std::string blockName = currentFunctionName + "_" + std::to_string(currIndex++);
	(*allSymbolTables)[blockName] = blockTable;

    visitChildren(ctx);

	// If the block's varOffset is less than the old symbol table's varOffset, update the old symbol table's varOffset
	if (blockTable->getVarOffset() < oldSymbolTable->getVarOffset()) {
    oldSymbolTable->setVarOffset(blockTable->getVarOffset());
	}

    currSymbolTable = oldSymbolTable;
    return 0;
}

std::any StaticAnalysisVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
	for (auto Node : ctx->assignStatement()) {
		std::string varName = Node->NAME()->getText();

		if (currSymbolTable->getLocalVariable(varName) != nullptr) {
			std::cerr << "Error: Variable '" << varName << "' has already been declared." << std::endl;
			hasError = true;
		} else {
			currSymbolTable->addVariable(varName);
		    visit(Node);
		}
	}
	return 0;
}


std::any StaticAnalysisVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
    if (ctx->expr()) {
        std::string varName = ctx->NAME()->getText();

        if (currSymbolTable->getVariable(varName) == nullptr) {
            std::cerr << "Error: Variable '" << varName << "' assigned before declaration." << std::endl;
            hasError = true;
        }

        // Visit the right side of the equals sign
        visit(ctx->expr());
    }

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
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			actualArgs = args->expr().size();
		}
	}

	if (expectedArgs >= 0 && actualArgs != expectedArgs) {
		std::cerr << "Error: Function '" << funcName << "' expects "
				  << expectedArgs << " argument(s), but " << actualArgs
				  << " were provided." << std::endl;
		hasError = true;
	}

	// Check: void function cannot be used in an expression
	if (it->second.returnType == VOID) {
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
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			actualArgs = args->expr().size();
		}
	}

	if (expectedArgs >= 0 && actualArgs != expectedArgs) {
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
