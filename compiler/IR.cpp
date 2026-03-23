#include "IR.h"
#include "CFG.h"

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
            output << "    subl %edx, %eax\n";
            break;
        
        case IRInstr::Operation::mul:
            output << "    imull %edx, %eax\n";
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
        
        case IRInstr::Operation::call:
            output << "    call " << params[0] << "\n";
            break;
        /* 
		cmp_eq,
		cmp_lt,
		cmp_le
        */
    }
}




