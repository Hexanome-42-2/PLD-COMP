#include "StaticAnalysisVisitor.h"
#include "generated/ifccLexer.h"

std::any StaticAnalysisVisitor::visitIncludeStatement(ifccParser::IncludeStatementContext *ctx) {
	std::string filePath = ctx->file->getText();
	// Remove surrounding quotes or angle brackets
	if ((filePath.front() == '"' && filePath.back() == '"') || (filePath.front() == '<' && filePath.back() == '>')) {
		filePath = filePath.substr(1, filePath.size() - 2);
	} else {
		std::cerr << "Error: Invalid include file format for '" << ctx->file->getText() << "'. Expected format: \"file.h\" or <file.h>." << std::endl;
		hasError = true;
		return 0;
	}

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
		// If we were to implement the actual inclusion of the file, we would read its contents and parse it here.
		// For now we'll manually add some common functions to the function signatures to allow testing of includes without implementing full file parsing.
		if (filePath.find("stdio") != std::string::npos) { // Add the functions [putchar, getchar]
			(*functionSignatures)["putchar"] = {Type::INT, 1, true, false};
			(*functionSignatures)["getchar"] = {Type::INT, 0, true, false};
		}
	}
	return 0;
}

std::any StaticAnalysisVisitor::visitProg(ifccParser::ProgContext *ctx) {
	currSymbolTable = programSymbolTable;

	// === Pass 1: Register all function signatures before analyzing bodies ===
	for (ifccParser::FonctionContext* funcCtx : ctx->fonction()) {
		std::string name;
		std::string retType;
		int paramCount = -1;
		bool isDef = false;

		ifccParser::FunctionDeclarationContext* decl = dynamic_cast<ifccParser::FunctionDeclarationContext*>(funcCtx);
		ifccParser::FunctionDefinitionContext* def = dynamic_cast<ifccParser::FunctionDefinitionContext*>(funcCtx);

	    Type retTypeEnum;
		if (decl) {
			name = decl->funcName->getText();
			retType = decl->functype->getText();
		    retTypeEnum = stringToType(retType);

			if (decl->parameters()) {
				ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(decl->parameters());
				if (params) paramCount = params->NAME().size();
			}
		} else if (def) {
			name = def->funcName->getText();
			retType = def->functype->getText();
		    retTypeEnum = stringToType(retType);
			isDef = true;

			if (def->parameters()) {
				ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(def->parameters());
				if (params) paramCount = params->NAME().size();
			}
		} else {
			continue;
		}

		if (isDef) {
			functionsWithBody.insert(name);
		}

	    if (!functionSignatures->count(name)) {
	        // Pass 1 only registers signatures; function bodies are marked as defined
	        // when visitFunctionDefinition actually visits them in pass 2.
	        (*functionSignatures)[name] = {retTypeEnum, paramCount, isVisitingInclude, false};
	    }
	}

	// === Pass 2: Visit all children for semantic analysis ===
	visitChildren(ctx);

	// A complete program must define main; otherwise linking fails and IFCC
	// would incorrectly accept a program GCC rejects.
	if (functionsWithBody.find("main") == functionsWithBody.end()) {
		std::cerr << "Error: Program must define a 'main' function." << std::endl;
		hasError = true;
	}

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

std::any StaticAnalysisVisitor::visitFunctionDeclaration(ifccParser::FunctionDeclarationContext *ctx) {
	std::string functionName = ctx->funcName->getText();

	if (allSymbolTables->find(functionName) != allSymbolTables->end()) {
		std::cerr << "Error: Function '" << functionName << "' has already been declared." << std::endl;
		hasError = true;
		return 0;
	}

	SymbolTable* functionTable = new SymbolTable();
	(*allSymbolTables)[functionName] = functionTable;

	if (ctx->parameters()) {
		ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters());
		if (params) {
			std::unordered_set<std::string> seenParams;
			for (antlr4::tree::TerminalNode* varNode : params->NAME()) {
				std::string paramName = varNode->getText();
				if (!seenParams.insert(paramName).second) {
					std::cerr << "Error: Duplicate parameter name '" << paramName
					          << "' in function '" << functionName << "'." << std::endl;
					hasError = true;
					continue;
				}
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
	SymbolTable* functionTable = nullptr;
	bool reusedExistingTable = false;

	if (allSymbolTables->find(functionName) != allSymbolTables->end()) {
	    FunctionSignature& sig = (*functionSignatures)[functionName];
	    if (sig.isDefined) {
	        std::cerr << "Error: Function '" << functionName << "' has already been defined." << std::endl;
	        hasError = true;
	        return 0;
	    } else {
	        functionTable = (*allSymbolTables)[functionName];
	        reusedExistingTable = true;
	    }
	}

    (*functionSignatures)[functionName].isDefined = true;

    if (!functionTable) {
	    functionTable = new SymbolTable();
	    (*allSymbolTables)[functionName] = functionTable;
    }
	SymbolTable* oldSymbolTable = currSymbolTable;

	currSymbolTable = functionTable;
	currIndex = 0; // Reset block index for new function
	currentFunctionName = functionName;

	if (ctx->parameters()) {
		ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters());
		if (params) {
			std::unordered_set<std::string> seenParams;
			for (antlr4::tree::TerminalNode* varNode : params->NAME()) {
				std::string paramName = varNode->getText();
				if (!seenParams.insert(paramName).second) {
					std::cerr << "Error: Duplicate parameter name '" << paramName
					          << "' in function '" << functionName << "'." << std::endl;
					hasError = true;
					continue;
				}

				// If this definition follows a declaration, params were already inserted.
				if (reusedExistingTable && functionTable->getLocalVariable(paramName) != nullptr) {
					continue;
				}

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

std::any StaticAnalysisVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
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
	auto it = functionSignatures->find(funcName);
	if (it == functionSignatures->end()) {
		std::cerr << "Error: Function '" << funcName << "' is not declared." << std::endl;
		hasError = true;
		return 0;
	} else if (!it->second.isExternal && functionsWithBody.find(funcName) == functionsWithBody.end()) {
		std::cerr << "Error: Function '" << funcName << "' is declared but not defined." << std::endl;
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
	auto it = functionSignatures->find(funcName);
	if (it == functionSignatures->end()) {
		std::cerr << "Error: Function '" << funcName << "' is not declared." << std::endl;
		hasError = true;
		return 0;
	} else if (!it->second.isExternal && functionsWithBody.find(funcName) == functionsWithBody.end()) {
		std::cerr << "Error: Function '" << funcName << "' is declared but not defined." << std::endl;
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
