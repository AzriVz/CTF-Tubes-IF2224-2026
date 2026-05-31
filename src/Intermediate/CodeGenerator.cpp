#include "CodeGenerator.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

CodeGenerator::CodeGenerator() : symbolTable(nullptr) {}

const std::vector<Instruction> &CodeGenerator::generate(ASTNode *ast, const SymbolTable &table) {
    symbolTable = &table;
    instructions.clear();
    errors.clear();

    generateNode(ast);

    symbolTable = nullptr;
    return instructions;
}

int CodeGenerator::emit(const std::string &op, int level, const std::string &argument) {
    int line = currentLine();
    instructions.push_back(Instruction(line, op, level, argument));
    return line;
}

int CodeGenerator::emit(const std::string &op, int level, int argument) {
    return emit(op, level, std::to_string(argument));
}

void CodeGenerator::patchArgument(int instructionIndex, int targetLine) {
    if (instructionIndex < 0 || instructionIndex >= (int)instructions.size()) {
        addError("Cannot patch invalid instruction index " + std::to_string(instructionIndex));
        return;
    }
    instructions[instructionIndex].argument = std::to_string(targetLine);
}

void CodeGenerator::generateNode(ASTNode *node) {
    if (!node)
        return;

    if (auto *program = dynamic_cast<ProgramNode *>(node)) {
        generateProgram(program);
    }
    else {
        generateStatement(node);
    }
}

void CodeGenerator::generateProgram(ProgramNode *node) {
    emit("INT", 0, globalFrameSize());

    if (node->body) {
        generateStatement(node->body);
    }

    emit("RET", 0, 0);
}

void CodeGenerator::generateStatement(ASTNode *node) {
    if (!node)
        return;

    if (auto *block = dynamic_cast<BlockNode *>(node)) {
        generateBlock(block);
    }
    else if (auto *list = dynamic_cast<DeclarationListNode *>(node)) {
        generateDeclarationList(list);
    }
    else if (auto *assign = dynamic_cast<AssignNode *>(node)) {
        generateAssignment(assign);
    }
    else if (auto *call = dynamic_cast<ProcCallNode *>(node)) {
        generateProcedureCall(call);
    }
    else if (auto *ifNode = dynamic_cast<IfNode *>(node)) {
        generateIf(ifNode);
    }
    else if (auto *whileNode = dynamic_cast<WhileNode *>(node)) {
        generateWhile(whileNode);
    }
    else if (auto *repeatNode = dynamic_cast<RepeatNode *>(node)) {
        generateRepeat(repeatNode);
    }
    else if (auto *forNode = dynamic_cast<ForNode *>(node)) {
        generateFor(forNode);
    }
    else if (dynamic_cast<EmptyNode *>(node) ||
             dynamic_cast<VarDeclNode *>(node) ||
             dynamic_cast<ConstDeclNode *>(node) ||
             dynamic_cast<TypeDeclNode *>(node) ||
             dynamic_cast<SubprogramDeclNode *>(node)) {
        return;
    }
    else {
        addError("Unsupported statement node: " + node->nodeName());
    }
}

void CodeGenerator::generateBlock(BlockNode *node) {
    for (auto child : node->children) {
        generateStatement(child);
    }
}

void CodeGenerator::generateDeclarationList(DeclarationListNode *node) {
    for (auto child : node->children) {
        generateStatement(child);
    }
}

void CodeGenerator::generateAssignment(AssignNode *node) {
    if (!node->value || !node->target) {
        addError("Invalid assignment node");
        return;
    }

    generateExpression(node->value);

    int address = runtimeAddress(node->target);
    if (address < 0) {
        addError("Unsupported assignment target: " + node->target->nodeName());
        return;
    }

    emit("STO", 0, address);
}

void CodeGenerator::generateProcedureCall(ProcCallNode *node) {
    std::string procName = lower(node->procName);

    if (procName == "writeln") {
        if (node->args.empty()) {
            emit("OPR", 0, 14);
            return;
        }

        for (std::size_t i = 0; i < node->args.size(); i++) {
            generateExpression(node->args[i]);
            emit("OPR", 0, i + 1 == node->args.size() ? 14 : 13);
        }
        return;
    }

    if (procName == "write") {
        for (auto arg : node->args) {
            generateExpression(arg);
            emit("OPR", 0, 13);
        }
        return;
    }

    addError("Procedure call '" + node->procName + "' is not supported by the intermediate generator yet");
}

void CodeGenerator::generateIf(IfNode *node) {
    if (!node->condition || !node->thenStmt) {
        addError("Invalid if node");
        return;
    }

    generateExpression(node->condition);
    int jpcLine = emit("JPC", 0, 0);

    generateStatement(node->thenStmt);

    if (node->elseStmt) {
        int jmpLine = emit("JMP", 0, 0);
        patchArgument(jpcLine, currentLine());
        generateStatement(node->elseStmt);
        patchArgument(jmpLine, currentLine());
    }
    else {
        patchArgument(jpcLine, currentLine());
    }
}

void CodeGenerator::generateWhile(WhileNode *node) {
    if (!node->condition || !node->body) {
        addError("Invalid while node");
        return;
    }

    int startLine = currentLine();
    generateExpression(node->condition);
    int jpcLine = emit("JPC", 0, 0);

    generateStatement(node->body);
    emit("JMP", 0, startLine);

    patchArgument(jpcLine, currentLine());
}

void CodeGenerator::generateRepeat(RepeatNode *node) {
    if (!node->body || !node->condition) {
        addError("Invalid repeat node");
        return;
    }

    int startLine = currentLine();
    generateStatement(node->body);
    generateExpression(node->condition);
    emit("JPC", 0, startLine);
}

void CodeGenerator::generateFor(ForNode *node) {
    int loopAddress = runtimeAddressByName(node->loopVar);
    if (loopAddress < 0 || !node->start || !node->end || !node->body) {
        addError("Invalid or unsupported for node");
        return;
    }

    generateExpression(node->start);
    emit("STO", 0, loopAddress);

    int startLine = currentLine();
    emit("LOD", 0, loopAddress);
    generateExpression(node->end);
    emit("OPR", 0, node->direction == "downto" ? 10 : 12);
    int jpcLine = emit("JPC", 0, 0);

    generateStatement(node->body);

    emit("LOD", 0, loopAddress);
    emit("LIT", 0, 1);
    emit("OPR", 0, node->direction == "downto" ? 3 : 2);
    emit("STO", 0, loopAddress);
    emit("JMP", 0, startLine);

    patchArgument(jpcLine, currentLine());
}

void CodeGenerator::generateExpression(ASTNode *node) {
    if (!node)
        return;

    if (auto *num = dynamic_cast<NumberNode *>(node)) {
        emit("LIT", 0, num->value);
    }
    else if (auto *real = dynamic_cast<RealNode *>(node)) {
        emit("LIT", 0, formatReal(real->value));
    }
    else if (auto *boolean = dynamic_cast<BooleanNode *>(node)) {
        emit("LIT", 0, boolean->value ? 1 : 0);
    }
    else if (auto *str = dynamic_cast<StringNode *>(node)) {
        emit("LIT", 0, quoteLiteral(str->value));
    }
    else if (auto *ch = dynamic_cast<CharNode *>(node)) {
        emit("LIT", 0, quoteLiteral(std::string(1, ch->value)));
    }
    else if (auto *var = dynamic_cast<VarNode *>(node)) {
        int address = runtimeAddress(var);
        if (address >= 0) {
            emit("LOD", 0, address);
        }
        else {
            int idx = var->tabRef;
            if (symbolTable && idx > 0 && idx < symbolTable->getTabSize()) {
                const auto &entry = symbolTable->getTabEntry(idx);
                if (entry.obj == "constant") {
                    emit("LIT", 0, entry.adr);
                    return;
                }
            }
            addError("Cannot load variable '" + var->name + "'");
        }
    }
    else if (auto *bin = dynamic_cast<BinOpNode *>(node)) {
        if (bin->op == "and") {
            generateExpression(bin->left);
            generateExpression(bin->right);
            emit("OPR", 0, 4);
            return;
        }

        if (bin->op == "or") {
            generateExpression(bin->left);
            generateExpression(bin->right);
            emit("OPR", 0, 2);
            emit("LIT", 0, 0);
            emit("OPR", 0, 11);
            return;
        }

        int operation = operationForBinary(bin->op);
        if (operation < 0) {
            addError("Unsupported binary operator '" + bin->op + "'");
            return;
        }

        generateExpression(bin->left);
        generateExpression(bin->right);
        emit("OPR", 0, operation);
    }
    else if (auto *unary = dynamic_cast<UnaryOpNode *>(node)) {
        generateExpression(unary->operand);

        if (unary->op == "-") {
            emit("OPR", 0, 1);
        }
        else if (unary->op == "+") {
            return;
        }
        else if (unary->op == "not") {
            emit("LIT", 0, 0);
            emit("OPR", 0, 7);
        }
        else {
            addError("Unsupported unary operator '" + unary->op + "'");
        }
    }
    else if (auto *call = dynamic_cast<FuncCallNode *>(node)) {
        addError("Function call '" + call->funcName + "' is not supported by the intermediate generator yet");
    }
    else if (dynamic_cast<ArrayAccessNode *>(node)) {
        addError("Array access is not supported by the intermediate generator yet");
    }
    else if (dynamic_cast<RecordAccessNode *>(node)) {
        addError("Record access is not supported by the intermediate generator yet");
    }
    else {
        addError("Unsupported expression node: " + node->nodeName());
    }
}

int CodeGenerator::currentLine() const {
    return (int)instructions.size();
}

int CodeGenerator::globalFrameSize() const {
    if (!symbolTable || symbolTable->getBtabSize() <= 0)
        return 3;
    return 3 + symbolTable->getBtabEntry(0).vsze;
}

int CodeGenerator::runtimeAddress(const ASTNode *node) const {
    if (!node || !symbolTable)
        return -1;

    if (dynamic_cast<const ArrayAccessNode *>(node) ||
        dynamic_cast<const RecordAccessNode *>(node)) {
        return -1;
    }

    int idx = node->tabRef;
    if (idx <= 0 || idx >= symbolTable->getTabSize())
        return -1;

    const auto &entry = symbolTable->getTabEntry(idx);
    if (entry.obj != "variable" && entry.obj != "parameter")
        return -1;

    return 3 + entry.adr;
}

int CodeGenerator::runtimeAddressByName(const std::string &name) const {
    if (!symbolTable)
        return -1;

    int idx = symbolTable->lookup(name);
    if (idx <= 0 || idx >= symbolTable->getTabSize())
        return -1;

    const auto &entry = symbolTable->getTabEntry(idx);
    if (entry.obj != "variable" && entry.obj != "parameter")
        return -1;

    return 3 + entry.adr;
}

int CodeGenerator::operationForBinary(const std::string &op) const {
    if (op == "+")
        return 2;
    if (op == "-")
        return 3;
    if (op == "*")
        return 4;
    if (op == "/" || op == "div")
        return 5;
    if (op == "mod")
        return 6;
    if (op == "=")
        return 7;
    if (op == "<>")
        return 8;
    if (op == "<")
        return 9;
    if (op == ">=")
        return 10;
    if (op == ">")
        return 11;
    if (op == "<=")
        return 12;
    return -1;
}

std::string CodeGenerator::formatReal(double value) const {
    std::ostringstream oss;
    oss << std::setprecision(15) << value;
    return oss.str();
}

std::string CodeGenerator::quoteLiteral(const std::string &value) const {
    std::string escaped;
    for (char ch : value) {
        if (ch == '\\' || ch == '\'')
            escaped.push_back('\\');
        escaped.push_back(ch);
    }
    return "'" + escaped + "'";
}

std::string CodeGenerator::lower(const std::string &value) const {
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return (char)std::tolower(ch); });
    return result;
}

void CodeGenerator::addError(const std::string &message) {
    errors.push_back("Code generation error: " + message);
}

void CodeGenerator::print(std::ostream &out) const {
    out << "=== Intermediate Code ===" << std::endl;
    for (const auto &instruction : instructions) {
        out << instruction << std::endl;
    }

    if (!errors.empty()) {
        out << std::endl;
        out << "=== Code Generation Errors ===" << std::endl;
        for (const auto &error : errors) {
            out << error << std::endl;
        }
    }
}
