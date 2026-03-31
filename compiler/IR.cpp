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
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";  // todo: change all kReturnReg with kScratchRegs
            output << "    addl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        
        case IRInstr::Operation::sub:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    subl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        
        case IRInstr::Operation::mul:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    imull " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::div:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    cltd\n";
            output << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::mod:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    cltd\n";
            output << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm("edx") << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";  // todo: maybe store the value of edx in params[0] to be sure to keep the parameters used
            break;

        case IRInstr::Operation::rmem:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
		
        case IRInstr::Operation::wmem:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;

        case IRInstr::Operation::negl:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    negl " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::call:
            output << "    call " << params[0] << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::plus:
            break;

		case IRInstr::Operation::notl:
            output << "    cmpl $0, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "	   sete	%al\n";
            output << "	   movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::band:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    andl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::bor:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    orl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::bxor:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    xorl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::cmp_eq:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    sete %al\n";
            output << "    movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_ne:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    setne %al\n";
            output << "    movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_lt:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    setl %al\n";
            output << "    movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_gt:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    setg %al\n";
            output << "    movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::jmp:
            output << "    jmp " << params[0] << "\n";
            break;
        case IRInstr::Operation::je:
            output << "    je " << params[0] << "\n";
            break;
    }
}




