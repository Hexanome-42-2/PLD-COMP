#include "IR.h"

void CFG::add_bb(BasicBlock* bb) {
    bbs.push_back(bb);
    current_bb = bb;
}

std::string CFG::new_BB_name() {
    std::string name = "main_bloc" + nextBBnumber;
    nextBBnumber++;
    return name;
}

void CFG::gen_asm(ostream& output) {
    gen_asm_prologue(output);
    for (BasicBlock* bb : bbs) {
        bb->gen_asm(output);
    }
    gen_asm_epilogue(output);
}

std::string CFG::IR_reg_to_asm(std::string reg) {
    if (reg == "eax" || reg == "edx") {
        return "%" + reg;
    } else {
        return symbolTable->getVariableOffset(reg) + "(%rbp)";
    }
}

void CFG::gen_asm_prologue(ostream& output) {
    #ifdef __APPLE__
    output << ".globl _main\n" ;
    output << " _main: \n" ;
    #else
    output << ".globl main\n" ;
    output << " main: \n" ;
    #endif
    output << "    pushq %rbp\n";
    output << "    movq %rsp, %rbp\n";
    output << "    subq $" << symbolTable->getMaxOffset() << ", %rsp\n";
}

void CFG::gen_asm_epilogue(ostream& output) {
    output << "    movq %rbp, %rsp\n";
	output << "    popq %rbp\n";
	output << "    ret \n";
}
/*
void CFG::add_to_symbol_table(std::string name, Type t) {
    symbolTable
}*/

std::string CFG::create_new_tempvar(Type t) {
    return symbolTable->addTemporaryVariable();
}

int CFG::get_var_index(std::string name) {
    return symbolTable->getVariableOffset();
}

Type CFG::get_var_type(std::string name) {
    return symbolTable->getVariableType();
}

void BasicBlock::gen_asm(ostream &output) {
    output << label << ":" << std::endl;
    for (IRInstr* instr : instrs) {
        instr->gen_asm(output);
    }
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params) {
    IRInstr * aIRInstr = new IRInstr(this, op, t, params);
    instrs.push_back(aIRInstr);
}
	
void IRInstr::gen_asm(ostream &output) {
    switch(IRInstr::Operation op) {
        case IRInstr::Operation::ldconst:
            output << "    movl $" << params[1] << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case IRInstr::Operation::copy:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        case IRInstr::Operation::add:
            output << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n"; 
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n";
            output << "    addl %edx, %eax\n";
            break;
        case IRInstr::Operation::sub:
            output << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n";
            output << "    subl %edx, %eax\n";
            break;
        case IRInstr::Operation::mul:
            output << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n";
            output << "    imull %edx, %eax\n";
            break;
        case IRInstr::Operation::rmem:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n";
            output << "    movl (%edx), " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
		/*wmem,
		call, 
		cmp_eq,
		cmp_lt,
		cmp_le*/
    }
}




