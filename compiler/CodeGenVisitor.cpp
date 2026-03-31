#include "CodeGenVisitor.h"


antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    visitChildren(ctx);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunction(ifccParser::FunctionContext *ctx) {
	// 1. Create a new CFG for this function and add it to the map
	std::string functionName = ctx->funcName->getText();
	currBlockIndex = 0; // Reset block index for new function
	CFG* oldCFG = currentCFG; // Save the current CFG to restore it later
	currentCFG = new CFG((*functionSymbolTables)[functionName], functionName);

    cfgContainer->add_cfg(functionName, currentCFG);

    currentCFG->add_bb(new BasicBlock(currentCFG, currentCFG->new_BB_name())); // Start with a new basic block for the function entry

	// Materialize incoming register arguments into local parameter slots.
	if (ctx->parameters()) {
		ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters());
		if (params) {
			std::vector<antlr4::tree::TerminalNode*> paramNames = params->NAME();
			for (size_t i = 0; i < paramNames.size() && i < kArgRegs.size(); ++i) {
				std::string paramName = paramNames[i]->getText();
				int offset = (*functionSymbolTables)[functionName]->getVariableOffset(paramName);
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT,
					{paramNames[i]->getText(), kArgRegs[i]});
			}
		}
	}

    // 2. Visit the function block to generate IR
    visit(ctx->block());

    // 3. Restore locations
    currentCFG = oldCFG; // Restore the previous CFG

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
    visitChildren(ctx);
    // Declaration only, no code to generate
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) {
	if (ctx->argument()) {
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			std::vector<std::string> argOffsets;
			for (ifccParser::ExprContext* argExpr : args->expr()) {
				const std::string tmpVar = std::any_cast<std::string>(visit(argExpr));
			    int offset = currentCFG->getRootSymbolTable()->getVariableOffset(tmpVar);
			    argOffsets.push_back(std::to_string(offset));
			}

			for (size_t i = 0; i < argOffsets.size() && i < kArgRegs.size(); ++i) {
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kArgRegs[i], argOffsets[i]});
			}
		}
	}

    std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText(), resVar});
	return resVar;
}

antlrcpp::Any CodeGenVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	// Visit the expression, evaluating the right side and putting the result in %eax
    std::string result;
    if (ctx->expr()) {
        result = std::any_cast<std::string>(visit(ctx->expr()));

        std::string varName = ctx->NAME()->getText();
        int offset = currentCFG->getSymbolTable()->getVariableOffset(varName);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kScratchRegs[0], result});
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {std::to_string(offset), kScratchRegs[0]});
    }

	return result;
}

antlrcpp::Any CodeGenVisitor::visitReturnStatement(ifccParser::ReturnStatementContext *ctx) {
	// Evaluate the result of the expression (if present — void functions have no return expr)
	if (ctx->expr()) {
		std::string result = std::any_cast<std::string>(visit(ctx->expr()));
	    currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kReturnReg, result});
	}

	// Add jump to function exit block
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::jmp, Type::VOID, {currentCFG->get_name() + "_exit"});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitIfStatement(ifccParser::IfStatementContext *ctx) {
    std::string statement_name = currentCFG->new_BB_name();
    BasicBlock *test_block = new BasicBlock(currentCFG, statement_name + "_test");
    BasicBlock *then_block = new BasicBlock(currentCFG, statement_name + "_then");
    BasicBlock *else_block = nullptr;

    int else_flag = (ctx->elst != nullptr || ctx->elbl != nullptr);
    if (else_flag) {
        else_block = new BasicBlock(currentCFG, statement_name + "_else");
    }

    BasicBlock *end_if_block = new BasicBlock(currentCFG, currentCFG->new_BB_name());

    currentCFG->add_bb(test_block);
    std::string resBool = std::any_cast<std::string>(visit(ctx->expr()));

    const std::string cstVar = currentCFG->create_new_tempvar(Type::INT);
    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    test_block->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {cstVar, "1"});
    test_block->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {resBool, cstVar, resVar});
    test_block->add_IRInstr(IRInstr::Operation::je, Type::VOID, {then_block->label});

    if (else_flag) {
        test_block->add_IRInstr(IRInstr::Operation::jmp, Type::VOID, {else_block->label});
    } else {
        test_block->add_IRInstr(IRInstr::Operation::jmp, Type::VOID, {end_if_block->label});
    }

    // True
    currentCFG->add_bb(then_block);
    test_block->exit_true = currentCFG->current_bb;

    if (ctx->ifbl != nullptr) {
        visit(ctx->ifbl);
    }
    if (ctx->ifst != nullptr) {
        visit(ctx->ifst);
    }

    currentCFG->current_bb->add_IRInstr(IRInstr::Operation::jmp, Type::VOID, {end_if_block->label});

    // False
    if (else_flag) {
        currentCFG->add_bb(else_block);
        test_block->exit_false = currentCFG->current_bb;

        if (ctx->elbl != nullptr) {
            visit(ctx->elbl);
        }
        if (ctx->elst != nullptr) {
            visit(ctx->elst);
        }

        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::jmp, Type::VOID, {end_if_block->label});
    }

    currentCFG->add_bb(end_if_block);

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitWhileStatement(ifccParser::WhileStatementContext *ctx) {
	std::string statement_name = currentCFG->new_BB_name();
    BasicBlock *test_block = new BasicBlock(currentCFG, statement_name + "_test");
    BasicBlock *then_block = new BasicBlock(currentCFG, statement_name + "_then");
    BasicBlock *end_while_block = new BasicBlock(currentCFG, currentCFG->new_BB_name());

    currentCFG->add_bb(test_block);
    std::string resBool = std::any_cast<std::string>(visit(ctx->expr()));

    const std::string cstVar = currentCFG->create_new_tempvar(Type::INT);
    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);

    test_block->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {cstVar, "1"});
    test_block->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {resBool, cstVar, resVar});

    test_block->add_IRInstr(IRInstr::Operation::je, Type::VOID, {then_block->label});
    test_block->add_IRInstr(IRInstr::Operation::jmp, Type::VOID, {end_while_block->label});

    // While loop
    currentCFG->add_bb(then_block);
    test_block->exit_true = currentCFG->current_bb;

    if (ctx->whbl != nullptr) {
        visit(ctx->whbl);
    }
    if (ctx->whst != nullptr) {
        visit(ctx->whst);
    }

    currentCFG->current_bb->add_IRInstr(IRInstr::Operation::jmp, Type::VOID, {test_block->label});

    currentCFG->add_bb(end_while_block);

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    std::string blockName = currentCFG->getName() + "_" + std::to_string(currBlockIndex++);

	std::cerr << "DEBUG CodeGen: looking for block '" << blockName << "'" << std::endl;

    SymbolTable* blockTable = (*functionSymbolTables)[blockName];
	std::cerr << "DEBUG CodeGen: blockTable = " << blockTable << std::endl;

    SymbolTable* oldTable = currentCFG->getSymbolTable();

    if (blockTable != nullptr) {
        currentCFG->setSymbolTable(blockTable);
    }

    visitChildren(ctx);

    currentCFG->setSymbolTable(oldTable);
    return 0;
}

// ~~~~~~~~ Expressions ~~~~~~~~

antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
	std::string expr = std::any_cast<std::string>(visit(ctx->expr_unary()));

	if (ctx->op) {
	    std::string resVar = currentCFG->create_new_tempvar(Type::INT);
        if (ctx->op->getText() == "-") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::negl, Type::INT, {expr, resVar});
        } else if (ctx->op->getText() == "+") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::plus, Type::INT, {});
        }else if (ctx->op->getText() == "!") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::notl, Type::INT, {expr, resVar});
        }

	    return resVar;
    }

	return expr;
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
	if (ctx->argument()) {
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			std::vector<std::string> argOffsets;
			for (ifccParser::ExprContext* argExpr : args->expr()) {
				const std::string tmpVar = std::any_cast<std::string>(visit(argExpr));
			    int offset = currentCFG->getRootSymbolTable()->getVariableOffset(tmpVar);
                argOffsets.push_back(std::to_string(offset));
			}

			for (size_t i = 0; i < argOffsets.size() && i < kArgRegs.size(); ++i) {
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kArgRegs[i], argOffsets[i]});
			}
		}
	}

    std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText(), resVar});
	return resVar;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
	std::string constValue = ctx->CONST()->getText();

    std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {tmpVar, constValue});

    return tmpVar;
}

antlrcpp::Any CodeGenVisitor::visitCharConstExpr(ifccParser::CharConstExprContext *ctx) {
	std::string text = ctx->CHAR_CONST()->getText(); // e.g. 'a' or '\n'
	// Strip surrounding single quotes: text[0] == '\'' and text[last] == '\''
	int value = 0;
	if (text.size() >= 3 && text[1] == '\\') {
		// Escape sequence
		switch (text[2]) {
			case 'n':  value = '\n'; break;
			case 't':  value = '\t'; break;
			case 'r':  value = '\r'; break;
			case '0':  value = '\0'; break;
			case '\\': value = '\\'; break;
			case '\'': value = '\''; break;
			case '\"': value = '\"'; break;
			case 'a':  value = '\a'; break;
			case 'b':  value = '\b'; break;
			case 'f':  value = '\f'; break;
			case 'v':  value = '\v'; break;
			default:   value = text[2]; break;
		}
	} else if (text.size() >= 3) {
		value = (unsigned char)text[1];
	}

    const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT,
		{tmpVar, std::to_string(value)});
	return tmpVar;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->NAME()->getText();
    int offset = currentCFG->getSymbolTable()->getVariableOffset(varName);

	return varName;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx) {
	std::string lExpr = std::any_cast<std::string>(visit(ctx->lExpr));
    std::string rExpr = std::any_cast<std::string>(visit(ctx->rExpr));

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    if (ctx->op->getText() == "+") {
    	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::add, Type::INT, {lExpr, rExpr, resVar});
    } else if (ctx->op->getText() == "-") {
    	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::sub, Type::INT, {lExpr, rExpr, resVar});
    }

	return resVar;
}

antlrcpp::Any CodeGenVisitor::visitMultDiv(ifccParser::MultDivContext *ctx) {
    std::string lExpr = std::any_cast<std::string>(visit(ctx->lExpr));
    std::string rExpr = std::any_cast<std::string>(visit(ctx->rExpr));

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    if (ctx->MULTOP()->getText() == "*") {
    	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mul, Type::INT, {lExpr, rExpr, resVar});
    } else if (ctx->MULTOP()->getText() == "/") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::div, Type::INT, {lExpr, rExpr, resVar});
    } else if (ctx->MULTOP()->getText() == "%") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mod, Type::INT, {lExpr, rExpr, resVar});
    }

	return resVar;
}

antlrcpp::Any CodeGenVisitor::visitBitWise(ifccParser::BitWiseContext *ctx) {
    std::string lExpr = std::any_cast<std::string>(visit(ctx->lExpr));
    std::string rExpr = std::any_cast<std::string>(visit(ctx->rExpr));

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    if (ctx->BITOP()->getText() == "&") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::band, Type::INT, {lExpr, rExpr, resVar});
    } else if (ctx->BITOP()->getText() == "|") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::bor, Type::INT, {lExpr, rExpr, resVar});
    } else if (ctx->BITOP()->getText() == "^") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::bxor, Type::INT, {lExpr, rExpr, resVar});
    }

    return resVar;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
	std::string expr = std::any_cast<std::string>(visit(ctx->expr()));

	return expr;
}

antlrcpp::Any CodeGenVisitor::visitComp(ifccParser::CompContext *ctx) {
    std::string lExpr = std::any_cast<std::string>(visit(ctx->lExpr));
    std::string rExpr = std::any_cast<std::string>(visit(ctx->rExpr));

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	if (ctx->COMPOP()->getText() == "<") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_lt, Type::INT, {lExpr, rExpr, resVar});
	} else if (ctx->COMPOP()->getText() == ">") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_gt, Type::INT, {lExpr, rExpr, resVar});
	}
	return resVar;
}

antlrcpp::Any CodeGenVisitor::visitEQ(ifccParser::EQContext *ctx) {
    std::string lExpr = std::any_cast<std::string>(visit(ctx->lExpr));
    std::string rExpr = std::any_cast<std::string>(visit(ctx->rExpr));

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	if (ctx->EQOP()->getText() == "==") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {lExpr, rExpr, resVar});
	} else if (ctx->EQOP()->getText() == "!=") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_ne, Type::INT, {lExpr, rExpr, resVar});
	}
	return resVar;
}
