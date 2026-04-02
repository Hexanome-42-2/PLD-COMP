#include "CodeGenVisitor.h"


antlrcpp::Any CodeGenVisitor::visitIncludeStatement(ifccParser::IncludeStatementContext *ctx) {
	// Do nothing
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    visitChildren(ctx);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunctionDeclaration(ifccParser::FunctionDeclarationContext *ctx) {
	// Do nothing for declarations, definitions will generate code
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunctionDefinition(ifccParser::FunctionDefinitionContext *ctx) {
	// 1. Create a new CFG for this function and add it to the map
	std::string functionName = ctx->funcName->getText();
	currBlockIndex = 0; // Reset block index for new function

	CFG* oldCFG = currentCFG; // Save the current CFG to restore it later
	currentCFG = new CFG((*functionSymbolTables)[functionName], functionName);

    cfgContainer->add_cfg(functionName, currentCFG);

    currentCFG->add_bb(new BasicBlock(currentCFG, currentCFG->new_BB_name())); // Start with a new basic block for the function entry

    declaredVars.clear();
    declaredVars.push_back({});
	// Materialize incoming register arguments into local parameter slots.
	if (ctx->parameters()) {
		ifccParser::ParamListContext* params = dynamic_cast<ifccParser::ParamListContext*>(ctx->parameters());
		if (params) {
			std::vector<antlr4::tree::TerminalNode*> paramNames = params->NAME();

			for (size_t i = 0; i < paramNames.size() && i < kArgRegs.size(); ++i) {
				std::string paramName = paramNames[i]->getText();
				int offset = (*functionSymbolTables)[functionName]->getVariableOffset(paramName);
			    declaredVars.back()[paramName] = offset;
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT,
					{std::to_string(offset), kArgRegs[i]});
			}
		}
	}

	// 2. Visit the function block to generate IR
	visitChildren(ctx->block());
    declaredVars.pop_back();

    // 3. Update max stack size
    currentCFG->getSymbolTable()->updateStackSize();

    // 4. Restore locations
    currentCFG = oldCFG; // Restore the previous CFG

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
    for (auto Node : ctx->assignStatement()) {
        std::string varName = Node->NAME()->getText();

        // get variable info from symbol table to find its offset
        VarInfo* info = currentCFG->getSymbolTable()->getLocalVariable(varName);

        if (!info) {
            std::cerr << "Error: Variable '" << varName << "' is not declared in the current scope." << std::endl;
            continue; // Skip this variable and continue with the next one
        }

        int offset = info->index;

        // Add the variable to the current scope's declared variables
        declaredVars.back()[varName] = offset;

        // Generate the initialization code if present
        if (Node->expr()) {
            ExprResult result = std::any_cast<ExprResult>(visit(Node->expr()));

            if (result.isConst) {
                result.expr = currentCFG->create_new_tempvar(Type::INT);
                currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {result.expr, std::to_string(result.value)});
            }

            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kScratchRegs[0], result.expr});
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {std::to_string(offset), kScratchRegs[0]});
        }
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) {
	if (ctx->argument()) {
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			std::vector<std::string> argExprs;
			for (ifccParser::ExprContext* argExpr : args->expr()) {
				ExprResult tmpVar = std::any_cast<ExprResult>(visit(argExpr));

			    if (tmpVar.isConst) {
			        tmpVar.expr = currentCFG->create_new_tempvar(Type::INT);
			        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {tmpVar.expr, std::to_string(tmpVar.value)});
			    }

			    argExprs.push_back(tmpVar.expr);
			}

			for (size_t i = 0; i < argExprs.size() && i < kArgRegs.size(); ++i) {
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kArgRegs[i], argExprs[i]});
			}
		}
	}

    std::string funcName = ctx->NAME()->getText();
    bool isExternal = (*functionSignatures)[funcName].isExternal;
    std::string external = isExternal ? "PLT" : "";

    std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText(), resVar, external});
	return ExprResult{false, -1, resVar};
}

antlrcpp::Any CodeGenVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	// Visit the expression, evaluating the right side and putting the result in %eax
    ExprResult result;
    if (ctx->expr()) {
        result = std::any_cast<ExprResult>(visit(ctx->expr()));

        if (result.isConst) {
            result.expr = currentCFG->create_new_tempvar(Type::INT);
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {result.expr, std::to_string(result.value)});
        }

        std::string varName = ctx->NAME()->getText();

        int offset = resolveVarOffset(varName);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kScratchRegs[0], result.expr});
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {std::to_string(offset), kScratchRegs[0]});
    }

    currentCFG->clear_temporary_variables();

	return ExprResult{false, -1, result.expr};
}

antlrcpp::Any CodeGenVisitor::visitReturnStatement(ifccParser::ReturnStatementContext *ctx) {
	// Evaluate the result of the expression (if present — void functions have no return expr)
	if (ctx->expr()) {
		ExprResult result = std::any_cast<ExprResult>(visit(ctx->expr()));

	    if (result.isConst) {
	        result.expr = currentCFG->create_new_tempvar(Type::INT);
	        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {result.expr, std::to_string(result.value)});
	    }

	    currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kReturnReg, result.expr});
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
    ExprResult resBool = std::any_cast<ExprResult>(visit(ctx->expr()));

    if (resBool.isConst) {
        resBool.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {resBool.expr, std::to_string(resBool.value)});
    }

    const std::string cstVar = currentCFG->create_new_tempvar(Type::INT);
    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    test_block->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {cstVar, "1"});
    test_block->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {resBool.expr, cstVar, resVar});
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
    ExprResult resBool = std::any_cast<ExprResult>(visit(ctx->expr()));

    if (resBool.isConst) {
        resBool.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {resBool.expr, std::to_string(resBool.value)});
    }

    const std::string cstVar = currentCFG->create_new_tempvar(Type::INT);
    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);

    test_block->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {cstVar, "1"});
    test_block->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {resBool.expr, cstVar, resVar});

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
    SymbolTable* blockTable = (*functionSymbolTables)[blockName];

    SymbolTable* oldTable = currentCFG->getSymbolTable();

    if (blockTable != nullptr) {
        currentCFG->setSymbolTable(blockTable);
    }


    declaredVars.push_back({});
    visitChildren(ctx);
    declaredVars.pop_back();

    if (blockTable != nullptr) {
        blockTable->updateStackSize();
    }
    currentCFG->setSymbolTable(oldTable);
    return 0;
}

// ~~~~~~~~ Expressions ~~~~~~~~

antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
	ExprResult expr = std::any_cast<ExprResult>(visit(ctx->expr_unary()));

	if (ctx->op) {
	    if (expr.isConst) {
            int value = expr.value;
            if (ctx->op->getText() == "-") {
                value = -value;
            } else if (ctx->op->getText() == "+") {
                // value remains the same
            } else if (ctx->op->getText() == "!") {
                value = !value;
            }
            return ExprResult{true, value, ""};
        }

	    std::string resVar = currentCFG->create_new_tempvar(Type::INT);
        if (ctx->op->getText() == "-") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::negl, Type::INT, {expr.expr, resVar});
        } else if (ctx->op->getText() == "+") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::plus, Type::INT, {expr.expr, resVar});
        }else if (ctx->op->getText() == "!") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::notl, Type::INT, {expr.expr, resVar});
        }

	    return ExprResult{false, -1, resVar};
    }

	return expr;
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
	if (ctx->argument()) {
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			std::vector<std::string> argExprs;
			for (ifccParser::ExprContext* argExpr : args->expr()) {
				ExprResult tmpVar = std::any_cast<ExprResult>(visit(argExpr));

			    if (tmpVar.isConst) {
			        tmpVar.expr = currentCFG->create_new_tempvar(Type::INT);
			        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {tmpVar.expr, std::to_string(tmpVar.value)});
			    }

                argExprs.push_back(tmpVar.expr);
			}

			for (size_t i = 0; i < argExprs.size() && i < kArgRegs.size(); ++i) {
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kArgRegs[i], argExprs[i]});
			}
		}
	}

    std::string funcName = ctx->NAME()->getText();
    bool isExternal = (*functionSignatures)[funcName].isExternal;
    std::string external = isExternal ? "PLT" : "";

    std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText(), resVar, external});
	return ExprResult{false, -1, resVar};
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
	std::string constValue = ctx->CONST()->getText();

    return ExprResult{true, stoi(constValue), ""};
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
	return ExprResult{true, value, tmpVar};
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->NAME()->getText();
    int offset = resolveVarOffset(varName);

	return ExprResult{false, -1, std::to_string(offset)};
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx) {
	ExprResult lExpr = std::any_cast<ExprResult>(visit(ctx->lExpr));
    ExprResult rExpr = std::any_cast<ExprResult>(visit(ctx->rExpr));

    if (lExpr.isConst && rExpr.isConst) {
        // If both sides are constant, we can compute the result at compile time
        int lValue = lExpr.value;
        int rValue = rExpr.value;
        if (ctx->op->getText() == "+") {
            int resultValue = lValue + rValue;

            return ExprResult{true, resultValue, ""};
        } else if (ctx->op->getText() == "-") {
            int resultValue = lValue - rValue;

            return ExprResult{true, resultValue, ""};
        }
    }

    if (lExpr.isConst) {
        lExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {lExpr.expr, std::to_string(lExpr.value)});
    }
    if (rExpr.isConst) {
        rExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {rExpr.expr, std::to_string(rExpr.value)});
    }

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    if (ctx->op->getText() == "+") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::add, Type::INT, {lExpr.expr, rExpr.expr, resVar});
    } else if (ctx->op->getText() == "-") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::sub, Type::INT, {lExpr.expr, rExpr.expr, resVar});
    }

	return ExprResult{false, -1, resVar};
}

antlrcpp::Any CodeGenVisitor::visitMultDiv(ifccParser::MultDivContext *ctx) {
    ExprResult lExpr = std::any_cast<ExprResult>(visit(ctx->lExpr));
    ExprResult rExpr = std::any_cast<ExprResult>(visit(ctx->rExpr));

    if (lExpr.isConst && rExpr.isConst) {
        // If both sides are constant, we can compute the result at compile time
        int lValue = lExpr.value;
        int rValue = rExpr.value;
        if (ctx->MULTOP()->getText() == "*") {
            int resultValue = lValue * rValue;

            return ExprResult{true, resultValue, ""};
        } else if (ctx->MULTOP()->getText() == "/" && (rValue != 0)) {
            int resultValue = lValue / rValue;

            return ExprResult{true, resultValue, ""};
        } else if (ctx->MULTOP()->getText() == "%" && (rValue != 0)) {
            int resultValue = lValue % rValue;

            return ExprResult{true, resultValue, ""};
        }
    }

    if (lExpr.isConst && !(ctx->MULTOP()->getText() == "*" && lExpr.value == 0)) {
        lExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {lExpr.expr, std::to_string(lExpr.value)});
    }
    if (rExpr.isConst && !(ctx->MULTOP()->getText() == "*" && rExpr.value == 0)) {
        rExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {rExpr.expr, std::to_string(rExpr.value)});
    }

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    if (ctx->MULTOP()->getText() == "*") {

        if (lExpr.isConst && lExpr.value == 0) {
            return ExprResult{true, 0, ""};
        } else if (rExpr.isConst && rExpr.value == 0) {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {resVar, "0"});
            return ExprResult{true, 0, ""};
        }
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mul, Type::INT, {lExpr.expr, rExpr.expr, resVar});

    } else if (ctx->MULTOP()->getText() == "/") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::div, Type::INT, {lExpr.expr, rExpr.expr, resVar});
    } else if (ctx->MULTOP()->getText() == "%") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mod, Type::INT, {lExpr.expr, rExpr.expr, resVar});
    }

    return ExprResult{false, -1, resVar};
}

antlrcpp::Any CodeGenVisitor::visitBitWise(ifccParser::BitWiseContext *ctx) {
    ExprResult lExpr = std::any_cast<ExprResult>(visit(ctx->lExpr));
    ExprResult rExpr = std::any_cast<ExprResult>(visit(ctx->rExpr));

    if (lExpr.isConst && rExpr.isConst) {
        // If both sides are constant, we can compute the result at compile time
        int lValue = lExpr.value;
        int rValue = rExpr.value;
        if (ctx->BITOP()->getText() == "&") {
            int resultValue = lValue & rValue;

            return ExprResult{true, resultValue, ""};
        } else if (ctx->BITOP()->getText() == "|") {
            int resultValue = lValue | rValue;

            return ExprResult{true, resultValue, ""};
        } else if (ctx->BITOP()->getText() == "^") {
            int resultValue = lValue ^ rValue;

            return ExprResult{true, resultValue, ""};
        }
    }

    if (lExpr.isConst) {
        lExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {lExpr.expr, std::to_string(lExpr.value)});
    }
    if (rExpr.isConst) {
        rExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {rExpr.expr, std::to_string(rExpr.value)});
    }

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
    if (ctx->BITOP()->getText() == "&") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::band, Type::INT, {lExpr.expr, rExpr.expr, resVar});
    } else if (ctx->BITOP()->getText() == "|") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::bor, Type::INT, {lExpr.expr, rExpr.expr, resVar});
    } else if (ctx->BITOP()->getText() == "^") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::bxor, Type::INT, {lExpr.expr, rExpr.expr, resVar});
    }

    return ExprResult{false, -1, resVar};
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
	ExprResult expr = std::any_cast<ExprResult>(visit(ctx->expr()));

	return expr;
}

antlrcpp::Any CodeGenVisitor::visitComp(ifccParser::CompContext *ctx) {
    ExprResult lExpr = std::any_cast<ExprResult>(visit(ctx->lExpr));
    ExprResult rExpr = std::any_cast<ExprResult>(visit(ctx->rExpr));

    if (lExpr.isConst && rExpr.isConst) {
        // If both sides are constant, we can compute the result at compile time
        int lValue = lExpr.value;
        int rValue = rExpr.value;
	    if (ctx->COMPOP()->getText() == "<") {
            int resultValue = lValue < rValue;

            return ExprResult{true, resultValue, ""};
	    } else if (ctx->COMPOP()->getText() == ">") {
            int resultValue = lValue > rValue;

            return ExprResult{true, resultValue, ""};
        }
    }

    if (lExpr.isConst) {
        lExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {lExpr.expr, std::to_string(lExpr.value)});
    }
    if (rExpr.isConst) {
        rExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {rExpr.expr, std::to_string(rExpr.value)});
    }

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	if (ctx->COMPOP()->getText() == "<") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_lt, Type::INT, {lExpr.expr, rExpr.expr, resVar});
	} else if (ctx->COMPOP()->getText() == ">") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_gt, Type::INT, {lExpr.expr, rExpr.expr, resVar});
	}
	return ExprResult{false, -1, resVar};
}

antlrcpp::Any CodeGenVisitor::visitEQ(ifccParser::EQContext *ctx) {
    ExprResult lExpr = std::any_cast<ExprResult>(visit(ctx->lExpr));
    ExprResult rExpr = std::any_cast<ExprResult>(visit(ctx->rExpr));

    if (lExpr.isConst && rExpr.isConst) {
        // If both sides are constant, we can compute the result at compile time
        int lValue = lExpr.value;
        int rValue = rExpr.value;
	    if (ctx->EQOP()->getText() == "==") {
            int resultValue = lValue == rValue;

            return ExprResult{true, resultValue, ""};
	    } else if (ctx->EQOP()->getText() == "!=") {
            int resultValue = lValue != rValue;

            return ExprResult{true, resultValue, ""};
        }
    }

    if (lExpr.isConst) {
        lExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {lExpr.expr, std::to_string(lExpr.value)});
    }
    if (rExpr.isConst) {
        rExpr.expr = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {rExpr.expr, std::to_string(rExpr.value)});
    }

    const std::string resVar = currentCFG->create_new_tempvar(Type::INT);
	if (ctx->EQOP()->getText() == "==") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {lExpr.expr, rExpr.expr, resVar});
	} else if (ctx->EQOP()->getText() == "!=") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_ne, Type::INT, {lExpr.expr, rExpr.expr, resVar});
	}
	return ExprResult{false, -1, resVar};
}
