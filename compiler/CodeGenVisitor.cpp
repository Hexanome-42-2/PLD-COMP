#include "CodeGenVisitor.h"


antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    visitChildren(ctx);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunction(ifccParser::FunctionContext *ctx) {
	// 1. Create a new CFG for this function and add it to the map
	std::string functionName = ctx->funcName->getText();
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
    // Declaration only, no code to generate
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareAssignStatement(ifccParser::DeclareAssignStatementContext *ctx) {
	// Evaluate the expression on the right side
	visit(ctx->expr());

	// Store result into the newly declared variable
	std::string varName = ctx->NAME()->getText();
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {varName, "eax"});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) {
	if (ctx->argument()) {
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			std::vector<std::string> argTemps;
			for (ifccParser::ExprContext* argExpr : args->expr()) {
				visit(argExpr);
				const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});
				argTemps.push_back(tmpVar);
			}

			for (size_t i = 0; i < argTemps.size() && i < kArgRegs.size(); ++i) {
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kArgRegs[i], argTemps[i]});
			}
		}
	}

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText()});
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	// Visit the expression, evaluating the right side and putting the result in %eax
	visit(ctx->expr());
	
	std::string varName = ctx->NAME()->getText();

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {varName, "eax"});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturnStatement(ifccParser::ReturnStatementContext *ctx) {
	// Evaluate the result of the expression (if present — void functions have no return expr)
	if (ctx->expr()) {
		visit(ctx->expr());
	}

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
    visit(ctx->expr());

    test_block->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {"edx", "1"});
    test_block->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {"eax", "edx"});
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
    visit(ctx->expr());

    test_block->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {"edx", "1"});
    test_block->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {"eax", "edx"});
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

// ~~~~~~~~ Expressions ~~~~~~~~

antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
	visit(ctx->expr_unary());
	if (ctx->op) {
        if (ctx->op->getText() == "-") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::negl, Type::INT, {});
        } else if (ctx->op->getText() == "+") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::plus, Type::INT, {});
        }else if (ctx->op->getText() == "!") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::notl, Type::INT, {});
        }
    }
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
	if (ctx->argument()) {
		ifccParser::ArgumentListContext* args = dynamic_cast<ifccParser::ArgumentListContext*>(ctx->argument());
		if (args) {
			std::vector<std::string> argTemps;
			for (ifccParser::ExprContext* argExpr : args->expr()) {
				visit(argExpr);
				const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});
				argTemps.push_back(tmpVar);
			}

			for (size_t i = 0; i < argTemps.size() && i < kArgRegs.size(); ++i) {
				currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {kArgRegs[i], argTemps[i]});
			}
		}
	}

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText()});
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
	std::string constValue = ctx->CONST()->getText();

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {"eax", constValue});

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->NAME()->getText();

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"eax", varName});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx) {
	visit(ctx->lExpr);
	const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});
    if (ctx->op->getText() == "+") {
    	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::add, Type::INT, {"eax", "edx"});
    } else if (ctx->op->getText() == "-") {
    	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::sub, Type::INT, {"eax", "edx"});
    }
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitMultDiv(ifccParser::MultDivContext *ctx) {
	visit(ctx->lExpr);
	const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);

    if (ctx->MULTOP()->getText() == "*") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});
    	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mul, Type::INT, {"eax", "edx"});
    } else  {
        const std::string rVar = currentCFG->create_new_tempvar(Type::INT);
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {rVar, "eax"});
        if (ctx->MULTOP()->getText() == "/") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::div, Type::INT, {tmpVar, rVar});
        } else if (ctx->MULTOP()->getText() == "%") {
            currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mod, Type::INT, {tmpVar, rVar});
        }
    }
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitWise(ifccParser::BitWiseContext *ctx) {
    visit(ctx->lExpr);
    const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

    visit(ctx->rExpr);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});

    if (ctx->BITOP()->getText() == "&") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::band, Type::INT, {"eax", "edx"});
    } else if (ctx->BITOP()->getText() == "|") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::bor, Type::INT, {"eax", "edx"});
    } else if (ctx->BITOP()->getText() == "^") {
        currentCFG->current_bb->add_IRInstr(IRInstr::Operation::bxor, Type::INT, {"eax", "edx"});
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
	visit(ctx->expr());
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitComp(ifccParser::CompContext *ctx) {
	visit(ctx->lExpr);
	const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});

	if (ctx->COMPOP()->getText() == "<") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_lt, Type::INT, {"eax", "edx"});
	} else if (ctx->COMPOP()->getText() == ">") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_gt, Type::INT, {"eax", "edx"});
	}
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitEQ(ifccParser::EQContext *ctx) {
		visit(ctx->lExpr);
	const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});

	if (ctx->EQOP()->getText() == "==") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_eq, Type::INT, {"eax", "edx"});
	} else if (ctx->EQOP()->getText() == "!=") {
		currentCFG->current_bb->add_IRInstr(IRInstr::Operation::cmp_ne, Type::INT, {"eax", "edx"});
	}
	return 0;
}
