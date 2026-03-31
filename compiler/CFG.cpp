#include "CFG.h"

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
    std::string bb_name = name + "_bloc_" + std::to_string(nextBBnumber);
    nextBBnumber++;
    return bb_name;
}

std::string CFG::IR_reg_to_asm(std::string reg) {
    #if defined(__x86_64__) || defined(_M_X64)
        if (isRegister(reg)) {
            return "%" + reg;
        } else {
            std::string ret =  std::to_string(symbolTable->getVariableOffset(reg)) + "(%rbp)";
            return ret;
        }
    #elif defined(__aarch64__) || defined(_M_ARM64)
        if (isRegister(reg)) {
            return reg;
        } else {
            std::string ret = "[sp, #" + std::to_string(-symbolTable->getVariableOffset(reg)) + "]";
            return ret;
        }
    #else
    #error Architecture not supported. Please define IR_reg_to_asm for your target architecture.
    #endif
}

void CFG::gen_asm(std::ostream& output) {
    gen_asm_prologue(output);
    for (BasicBlock* bb : bbs) {
        bb->gen_asm(output);
    }
    gen_asm_epilogue(output);
}

void CFG::gen_asm_prologue(std::ostream& output) {
    #ifdef __APPLE__
        output << ".globl _" << name << "\n" ;
        output << "_" << name << ": \n" ;
    #else
        output << ".globl " << name << "\n" ;
        output << name << ": \n" ;
    #endif

    #if defined(__x86_64__) || defined(_M_X64)
        output << "    pushq %rbp\n";
        output << "    movq %rsp, %rbp\n";
        output << "    subq $" << kStackFrameSize << ", %rsp\n";
    #elif defined(__aarch64__) || defined(_M_ARM64)
        output << "    push {fp, lr}\n";
        output << "    add	fp, sp, #0\n";
        output << "    sub	sp, sp, #"<< kStackFrameSize << "\n";
    #else
        #error Architecture not supported. Please define gen_asm_prologue for your target architecture.
    #endif
}

void CFG::gen_asm_epilogue(std::ostream& output) {
    output << name << "_exit:\n";

    #if defined(__x86_64__) || defined(_M_X64)
        output << "    movq %rbp, %rsp\n";
        output << "    popq %rbp\n";
        output << "    ret \n";
    #elif defined(__aarch64__) || defined(_M_ARM64)
        output << "    add sp, fp, #0\n";
        output << "    pop  {fp, lr}\n";
        output << "    bx lr\n";
    #else
        #error Architecture not supported. Please define gen_asm_epilogue for your target architecture.
    #endif
}

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


CFGContainer::~CFGContainer() {
    for (auto& pair : cfgs) {
        delete pair.second;
    }
    cfgs.clear();
}

void CFGContainer::add_cfg(std::string name, CFG* cfg) {
    cfgs[name] = cfg;
}

CFG* CFGContainer::get_cfg(std::string name) {
    auto it = cfgs.find(name);
    if (it != cfgs.end()) {
        return it->second;
    }
    return nullptr; // CFG not found
}

void CFGContainer::gen_asm(std::ostream &output) {
    gen_asm_prologue(output);
    for (const std::pair<std::string, CFG*> cfg : cfgs) {
        cfg.second->gen_asm(output);
    }
    gen_asm_epilogue(output);
}