#include "IR.h"

CFG::~CFG() {
    for (BasicBlock* bb : bbs) {
        delete bb;
    }
    bbs.clear();
}

void CFG::add_bb(BasicBlock* bb) {
    bbs.push_back(bb);
    current_bb = bb;
}

std::string CFG::new_BB_name() {
    std::string name = "main_bloc" + nextBBnumber;
    nextBBnumber++;
    return name;
}

void CFG::gen_asm(std::ostream& output) {
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
        std::string ret =  std::to_string(symbolTable->getVariableOffset(reg)) + "(%rbp)";
        return ret;
    }
}

void CFG::gen_asm_prologue(std::ostream& output) {
    #ifdef __APPLE__
    output << ".globl _main\n" ;
    output << "_main: \n" ;
    #else
    output << ".globl main\n" ;
    output << "main: \n" ;
    #endif
    output << "    pushq %rbp\n";
    output << "    movq %rsp, %rbp\n";
    output << "    subq $" << symbolTable->getMaxOffset() << ", %rsp\n";
}

void CFG::gen_asm_epilogue(std::ostream& output) {
    output << "main_exit:\n";
    output << "    movq %rbp, %rsp\n";
	output << "    popq %rbp\n";
	output << "    ret \n";
}
/*
void CFG::add_to_symbol_table(std::string name, Type t) {
    symbolTable
}*/

std::string CFG::create_new_tempvar(Type t) {
    std::string tmpVar = symbolTable->addTemporaryVariable();
    symbolTable->updateMaxOffset();
    return tmpVar;
}

int CFG::get_var_index(std::string name) {
    return symbolTable->getVariableOffset(name);
}

Type CFG::get_var_type(std::string name) {
    return symbolTable->getVariableType(name);
}

BasicBlock::~BasicBlock() {
    for (IRInstr* instr : instrs) {
        delete instr;
    }
    instrs.clear();
}

void BasicBlock::gen_asm(std::ostream &output) {
    output << label << ":" << std::endl;
    for (IRInstr* instr : instrs) {
        instr->gen_asm(output);
    }
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, std::vector<std::string> params) {
    IRInstr * aIRInstr = new IRInstr(this, op, t, params);
    instrs.push_back(aIRInstr);
}
	
void IRInstr::gen_asm(std::ostream &output) {
    switch(this->op) {
        case IRInstr::Operation::ldconst:
            output << "    movl $" << params[1] << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        
        case IRInstr::Operation::copy:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        
        case IRInstr::Operation::add:
            output << "    addl %edx, %eax\n";
            break;
        
        case IRInstr::Operation::sub:
            output << "    subl %eax, %edx\n";
            output << "    movl %edx, %eax\n";
            break;
        
        case IRInstr::Operation::mul:
            output << "    imull %edx, %eax\n";
            break;
        
        case IRInstr::Operation::div:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", %eax\n";
            output << "    cltd\n";
            output << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::mod:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", %eax\n";
            output << "    cltd\n";
            output << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    movl %edx, %eax\n";
            break;

        case IRInstr::Operation::rmem:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", %edx\n";
            output << "    movl %edx, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
		
        case IRInstr::Operation::wmem:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;

        case IRInstr::Operation::negl:
            //output << "    movl %eax, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    negl %eax\n";
            break;

        case IRInstr::Operation::plus:
            break;
        
		case IRInstr::Operation::notl: //FAUUUUUUUX
            output << "    movl %eax, %edx\n";
            output << "    xorl $1, %edx\n";
            output << "    movl %edx, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        /*    call, 
		cmp_eq,
		cmp_lt,
		cmp_le*/
    }
}




