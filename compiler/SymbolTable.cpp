#include "SymbolTable.h"

// Converts parsed type text to internal enum used by semantic/codegen passes
Type stringToType(const std::string& typeStr) {
    if (typeStr == "int") {
        return Type::INT;
    } else if (typeStr == "void") {
        return Type::VOID;
    } else {
        return Type::ERROR; // Invalid type
    }
}

// Returns byte size of an internal type for stack/frame calculations
int typeSizeOf(Type type) {
    switch (type) {
        case Type::INT:
            return 4;
        case Type::VOID:
            return 0; // Void has no size
        default:
            return 0;
    }
}


SymbolTable::~SymbolTable() {}

// Reserves a new local variable slot in the current stack frame
void SymbolTable::addVariable(const std::string &name) {
    // We grow stack slots downward from rbp, so each new local decrements offset
    varOffset -= 4; // Decrement offset for the next variable
    symbolTable[name] = {varOffset, Type::INT, false}; // Store the current offset and mark as unused

    // Ensure frame size accounts for regular variables (including params).
    if (varOffset < maxOffset) {
        maxOffset = varOffset;
    }
}

// Reserves a new temporary slot used by intermediate expression values
const std::string SymbolTable::addTemporaryVariable() {
    // Temps share the same frame space model as locals, but from tmpOffset
    tmpOffset -= 4; // Decrement offset for the next temporary variable
    std::string tmpName = nameTemporaryVariable(tmpOffset); // Create a unique name for the temporary variable
    symbolTable[tmpName] = {tmpOffset, Type::INT, false}; // Store the current offset and mark as unused
    return tmpName; // Return the name of the temporary variable
}

// Builds a deterministic internal name for a temporary slot offset
std::string SymbolTable::nameTemporaryVariable(int offset) {
    return "!tmp" + std::to_string(offset);
}

// Looks up a variable in this scope, then recursively in parent scopes
VarInfo *SymbolTable::getVariable(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return &(it->second); // Return a pointer to the VarInfo
    }
    if (parent != nullptr) {
        return parent->getVariable(name); // Variable not found, check parent
    }
    return nullptr; // Variable not found
}

// Looks up a variable only in the current scope (no parent traversal)
VarInfo* SymbolTable::getLocalVariable(const std::string& name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return &(it->second);
    }
    return nullptr; 
}

// Resolves a variable to its stack offset, searching parent scopes if needed
int SymbolTable::getVariableOffset(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.index;
    }
    if (parent != nullptr) {
        return parent->getVariableOffset(name); // Variable not found, check parent
    }
    return 1; // Variable not found
}

// Resolves a variable to its type, searching parent scopes if needed
Type SymbolTable::getVariableType(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.type;
    }
    if (parent != nullptr) {
        return parent->getVariableType(name); // Variable not found, check parent
    }
    return Type::ERROR; // Variable not found
}

// Resets temporary allocation cursor so new expressions can reuse temp slots
void SymbolTable::clearTemporaryVariables() {
    tmpOffset = varOffset;
}

// Returns deepest offset reached so caller can size stack frame correctly
int SymbolTable::getMaxOffset() const {
    return maxOffset;
}

// Checks whether a variable has been marked as used in this scope
bool SymbolTable::getUsed(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.used;
    }
    if (parent != nullptr) {
        return parent->getUsed(name); // Variable not found, check parent
    }
    return false; // Variable not found
}

// Marks a variable as used in the current table (used for warnings)
void SymbolTable::MarkUsed(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        it->second.used = true; // Mark the variable as used
    }
}

// Initializes temporary offset after declarations are known
void SymbolTable::InitializeTmpOffset() {
    // After semantic pass, temps start right below the last declared variable.
    tmpOffset = varOffset; // Start temporary variable offsets from the current variable offset
}

// Recomputes maxOffset considering both declared vars and temporaries
void SymbolTable::updateMaxOffset() {
    if (varOffset < maxOffset) maxOffset = varOffset;
    if (tmpOffset < maxOffset) maxOffset = tmpOffset;
}

// Collects names of currently unused symbols for warning reporting
std::vector<std::string> SymbolTable::getUnusedVariables() {
    std::vector<std::string> unusedVars;
    for (const auto& pair : symbolTable) {
        if (!pair.second.used) {
            unusedVars.push_back(pair.first); // Add the name of the unused variable to the list
        }
    }
    return unusedVars;
}

// Debug helper: prints symbols of this scope (and parent scopes first)
void SymbolTable::printSymbolTable() const {
    if (parent != nullptr) {
        parent->printSymbolTable();
        std::cout << "New scope:\n";
    } else {
        std::cout << "Symbol Table:\n";
    }
    for (const auto& pair : symbolTable) {
        std::cout << "Variable: " << pair.first << ", Offset: " << pair.second.index << ", Used: " << pair.second.used << "\n";
    }
}

// Finalizes required stack size for this scope and propagates max need to parent
void SymbolTable::updateStackSize() {
    stackSize -= maxOffset;
    if (parent != nullptr) {
        // Nested blocks do not allocate a new frame: parent keeps the max requirement.
        // Update the parent's stack size if this scope requires more space
        parent->stackSize = std::max(parent->stackSize, stackSize);
    }
}