#include "IR.h"
#include "CFG.h"

// Frees all IR instructions owned by this basic block when block is destroyed.
BasicBlock::~BasicBlock() {
    for (IRInstr* instr : instrs) {
        delete instr;
    }
    instrs.clear();
}

// Emits assembly for this block label and each IR instruction in order.
void BasicBlock::gen_asm(std::ostream &output) {
    output << label << ":" << std::endl;
    for (IRInstr* instr : instrs) {
        #if (defined(__x86_64__) || defined(_M_X64) || defined(DEV_ARCH_X86_64)) && not defined(DEV_ARCH_ARM64)
            instr->gen_asm_x86(output);
        #elif (defined(__aarch64__) || defined(_M_ARM64) || defined(DEV_ARCH_ARM64)) && not defined(DEV_ARCH_X86_64)
            instr->gen_asm_arm(output);
        #else
            #error Architecture not supported. Please define gen_asm for your target architecture.
        #endif

    }
}

// Appends one IR instruction node to this basic block.
void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, std::vector<std::string> params) {
    IRInstr * aIRInstr = new IRInstr(this, op, t, params);
    instrs.push_back(aIRInstr);
}
	
// Translates one IR instruction to x86 assembly
void IRInstr::gen_asm_x86(std::ostream &output) {
    switch(this->op) {
        // todo : optimise if one of the params is a reg
        case IRInstr::Operation::ldconst:
            output << "    movl $" << params[1] << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        
        case IRInstr::Operation::copy:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
        
        case IRInstr::Operation::add:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    addl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        
        case IRInstr::Operation::sub:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    subl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        
        case IRInstr::Operation::mul:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    imull " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::div:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            // x86 signed divide uses edx:eax as dividend, so cltd sign-extends eax into edx
            output << "    cltd\n";
            output << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::mod:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    cltd\n";
            output << "    idivl " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            // Remainder comes from edx after idiv
            output << "    movl %edx, " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::rmem:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;
		
        case IRInstr::Operation::wmem:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;

        case IRInstr::Operation::negl:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    negl " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::call:
            output << "    call " << params[0];
            if (params.size() > 2 && params[2] != "") {
                output << "@" << params[2];
            }
            output << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::plus:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

		case IRInstr::Operation::notl:
            output << "    cmpl $0, " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "	   sete	%al\n";
            // set* only writes 8 bits, movzbl cleans upper bits to get a real 0/1 int
            output << "	   movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::band:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    andl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::bor:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    orl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::bxor:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    xorl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::cmp_eq:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    sete %al\n";
            output << "    movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_ne:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    setne %al\n";
            output << "    movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_lt:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    setl %al\n";
            output << "    movzbl %al, " << bb->cfg->IR_reg_to_asm(kReturnReg) << "\n";
            output << "    movl " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_gt:
            output << "    movl " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    cmpl " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
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

// Translates one IR instruction to ARM assembly
void IRInstr::gen_asm_arm(std::ostream &output) {
    switch(this->op) {
        case IRInstr::Operation::ldconst:
            output << "    mov " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", #" << params[1] << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;

        case IRInstr::Operation::copy:
            output << "    cpy " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::add:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    add " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::sub:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    sub " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::mul:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    mul " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::div:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    sdiv " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::mod:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    sdiv " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            // mls computes r = a - q*b (modulo) after quotient q from sdiv
            output << "    mls " << bb->cfg->IR_reg_to_asm(kScratchRegs[3]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[3]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::rmem:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(params[0]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::wmem:
            output << "    str " << bb->cfg->IR_reg_to_asm(params[1]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            break;

        case IRInstr::Operation::negl:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    rsb " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", #0\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::call:
            output << "    bl " << params[0] << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kReturnReg) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::plus:
            break;

		case IRInstr::Operation::notl:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    cmp " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", #0\n";
            output << "    moveq " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", #1\n";
            output << "    movne " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", #0\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            break;

        case IRInstr::Operation::band:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    and " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::bor:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    orr " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::bxor:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    eor " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;

        case IRInstr::Operation::cmp_eq:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    cmp " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    moveq " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #1\n";
            output << "    movne " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #0\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_ne:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    cmp " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    movne " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #1\n";
            output << "    moveq " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #0\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_lt:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    cmp " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    movlt " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #1\n";
            output << "    movge " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #0\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::cmp_gt:
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(params[0]) << "\n";
            output << "    ldr " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << ", " << bb->cfg->IR_reg_to_asm(params[1]) << "\n";
            output << "    cmp " << bb->cfg->IR_reg_to_asm(kScratchRegs[0]) << ", " << bb->cfg->IR_reg_to_asm(kScratchRegs[1]) << "\n";
            output << "    movgt " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #1\n";
            output << "    movle " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", #0\n";
            output << "    str " << bb->cfg->IR_reg_to_asm(kScratchRegs[2]) << ", " << bb->cfg->IR_reg_to_asm(params[2]) << "\n";
            break;
        case IRInstr::Operation::jmp:
            output << "    b " << params[0] << "\n";
            break;
        case IRInstr::Operation::je:
            output << "    beq " << params[0] << "\n";
            break;
    }
}
