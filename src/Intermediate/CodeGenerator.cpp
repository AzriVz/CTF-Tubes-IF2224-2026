#include "CodeGenerator.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

using namespace std;

CodeGenerator::CodeGenerator() : symbolTable(nullptr), currentFrameLevel(0) {}

const vector<Instruction> &CodeGenerator::generate(ASTNode *ast, const SymbolTable &table) {
    symbolTable = &table;
    instructions.clear();
    errors.clear();
    subprogramEntries.clear();
    subprogramEntriesByName.clear();
    subprogramParamCounts.clear();
    subprogramParamCountsByName.clear();
    subprogramParamSlots.clear();
    subprogramParamSlotsByName.clear();
    currentFrameLevel = 0;

    generateNode(ast);

    symbolTable = nullptr;
    return instructions;
}

int CodeGenerator::emit(const string &op, int level, const string &argument) {
    int line = currentLine();
    instructions.push_back(Instruction(line, op, level, argument));
    return line;
}

int CodeGenerator::emit(const string &op, int level, int argument) {
    return emit(op, level, to_string(argument));
}

void CodeGenerator::patchArgument(int instructionIndex, int targetLine) {
    if (instructionIndex < 0 || instructionIndex >= (int)instructions.size()) {
        addError("Cannot patch invalid instruction index " + to_string(instructionIndex));
        return;
    }
    instructions[instructionIndex].argument = to_string(targetLine);
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
    currentFrameLevel = 0;
    emit("INT", 0, globalFrameSize());

    if (node->declarations) {
        generateStatement(node->declarations);
    }

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
    else if (auto *caseNode = dynamic_cast<CaseNode *>(node)) {
        generateCase(caseNode);
    }
    else if (auto *subprogram = dynamic_cast<SubprogramDeclNode *>(node)) {
        generateSubprogramDeclaration(subprogram);
    }
    else if (dynamic_cast<EmptyNode *>(node) ||
             dynamic_cast<VarDeclNode *>(node) ||
             dynamic_cast<ConstDeclNode *>(node) ||
             dynamic_cast<TypeDeclNode *>(node)) {
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

    int targetSize = storageSize(node->target);
    int valueSize = storageSize(node->value);
    if (targetSize > 1 || valueSize > 1) {
        if (targetSize != valueSize) {
            addError("Structured assignment size mismatch: target has " + to_string(targetSize) +
                     " slot(s), value has " + to_string(valueSize) + " slot(s)");
            return;
        }

        copyStructuredValue(node->value, node->target, targetSize);
        return;
    }

    generateExpression(node->value);
    storeTopToTarget(node->target);
}

void CodeGenerator::generateProcedureCall(ProcCallNode *node) {
    string procName = lower(node->procName);

    if (procName == "writeln") {
        if (node->args.empty()) {
            emit("LIT", 0, quoteLiteral(""));
            emit("OPR", 0, 14);
            return;
        }

        for (size_t i = 0; i < node->args.size(); i++) {
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

    if (procName == "readln") {
        for (auto arg : node->args) {
            generateStoreInput(arg);
        }
        return;
    }

    generateSubprogramCall(node->procName, node->tabRef, node->args, false);
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
    RuntimeLocation loopLocation = runtimeLocationByName(node->loopVar);
    if (!loopLocation.isValid() || !node->start || !node->end || !node->body) {
        addError("Invalid or unsupported for node");
        return;
    }

    generateExpression(node->start);
    emit("STO", loopLocation.levelDiff, loopLocation.address);

    int startLine = currentLine();
    emit("LOD", loopLocation.levelDiff, loopLocation.address);
    generateExpression(node->end);
    emit("OPR", 0, node->direction == "downto" ? 10 : 12);
    int jpcLine = emit("JPC", 0, 0);

    generateStatement(node->body);

    emit("LOD", loopLocation.levelDiff, loopLocation.address);
    emit("LIT", 0, 1);
    emit("OPR", 0, node->direction == "downto" ? 3 : 2);
    emit("STO", loopLocation.levelDiff, loopLocation.address);
    emit("JMP", 0, startLine);

    patchArgument(jpcLine, currentLine());
}

void CodeGenerator::generateCase(CaseNode *node) {
    if (!node->selector) {
        addError("Invalid case node");
        return;
    }

    vector<int> endJumps;

    for (auto branch : node->branches) {
        if (!branch)
            continue;

        for (auto label : branch->labels) {
            if (!label)
                continue;

            generateExpression(node->selector);
            generateExpression(label);
            emit("OPR", 0, 7);
            int nextLabel = emit("JPC", 0, 0);

            generateStatement(branch->statement);
            endJumps.push_back(emit("JMP", 0, 0));
            patchArgument(nextLabel, currentLine());
        }
    }

    for (int jumpLine : endJumps) {
        patchArgument(jumpLine, currentLine());
    }
}

void CodeGenerator::generateSubprogramDeclaration(SubprogramDeclNode *node) {
    if (!node->body) {
        addError("Subprogram '" + node->subName + "' has no body");
        return;
    }

    int skipLine = emit("JMP", 0, 0);
    int entryLine = currentLine();
    subprogramEntries[node->tabRef] = entryLine;
    subprogramEntriesByName[lower(node->subName)] = entryLine;
    subprogramParamCounts[node->tabRef] = (int)node->params.size();
    subprogramParamCountsByName[lower(node->subName)] = (int)node->params.size();
    int paramSlots = 0;
    for (auto param : node->params) {
        paramSlots += storageSize(param);
    }
    subprogramParamSlots[node->tabRef] = paramSlots;
    subprogramParamSlotsByName[lower(node->subName)] = paramSlots;

    int previousFrameLevel = currentFrameLevel;
    currentFrameLevel = node->level + 1;

    emit("INT", 0, frameSizeForBlock(node->body));
    generateStatement(node->body);
    emit("RET", 0, 0);

    currentFrameLevel = previousFrameLevel;
    patchArgument(skipLine, currentLine());
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
        emit("LIT", 0, quoteLiteral(string(1, ch->value)));
    }
    else if (auto *var = dynamic_cast<VarNode *>(node)) {
        if (isStructured(var)) {
            addError("Structured variable '" + var->name + "' requires an address/copy context");
            return;
        }

        RuntimeLocation location = runtimeLocation(var);
        if (location.isValid()) {
            emit("LOD", location.levelDiff, location.address);
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
        generateSubprogramCall(call->funcName, call->tabRef, call->args, true);
    }
    else if (dynamic_cast<ArrayAccessNode *>(node) ||
             dynamic_cast<RecordAccessNode *>(node)) {
        if (isStructured(node)) {
            addError("Structured access '" + node->nodeName() + "' requires an address/copy context");
            return;
        }

        if (generateAddress(node)) {
            emit("LDI", 0, 0);
        }
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

int CodeGenerator::frameSizeForBlock(const ASTNode *node) const {
    const BlockNode *block = dynamic_cast<const BlockNode *>(node);
    if (!symbolTable || !block)
        return 3;

    int blockIndex = block->blockIndex;
    if (blockIndex < 0 || blockIndex >= symbolTable->getBtabSize())
        return 3;

    const BtabEntry &entry = symbolTable->getBtabEntry(blockIndex);
    return 3 + entry.psze + entry.vsze;
}

CodeGenerator::RuntimeLocation CodeGenerator::runtimeLocation(const ASTNode *node) const {
    if (!node || !symbolTable)
        return RuntimeLocation();

    if (dynamic_cast<const ArrayAccessNode *>(node) ||
        dynamic_cast<const RecordAccessNode *>(node)) {
        return RuntimeLocation();
    }

    int idx = node->tabRef;
    if (idx <= 0 || idx >= symbolTable->getTabSize())
        return RuntimeLocation();

    const TabEntry &entry = symbolTable->getTabEntry(idx);
    if (entry.obj != "variable" && entry.obj != "parameter" && entry.obj != "function")
        return RuntimeLocation();

    int levelDiff = currentFrameLevel - entry.lev;
    if (levelDiff < 0)
        return RuntimeLocation();

    return RuntimeLocation(levelDiff, 3 + entry.adr);
}

CodeGenerator::RuntimeLocation CodeGenerator::runtimeLocationByName(const string &name) const {
    if (!symbolTable)
        return RuntimeLocation();

    int bestIdx = -1;
    int bestLevel = -1;
    for (int i = 0; i < symbolTable->getTabSize(); i++) {
        const TabEntry &entry = symbolTable->getTabEntry(i);
        if (entry.id == name &&
            (entry.obj == "variable" || entry.obj == "parameter") &&
            entry.lev <= currentFrameLevel &&
            entry.lev >= bestLevel) {
            bestIdx = i;
            bestLevel = entry.lev;
        }
    }

    if (bestIdx < 0)
        return RuntimeLocation();

    const TabEntry &entry = symbolTable->getTabEntry(bestIdx);
    return RuntimeLocation(currentFrameLevel - entry.lev, 3 + entry.adr);
}

bool CodeGenerator::generateAddress(ASTNode *node) {
    if (!node) {
        addError("Cannot generate address for null node");
        return false;
    }

    if (auto *var = dynamic_cast<VarNode *>(node)) {
        RuntimeLocation location = runtimeLocation(var);
        if (!location.isValid()) {
            addError("Cannot take address of variable '" + var->name + "'");
            return false;
        }

        emit("LDA", location.levelDiff, location.address);
        return true;
    }

    if (auto *arrayAccess = dynamic_cast<ArrayAccessNode *>(node)) {
        if (!arrayAccess->index) {
            addError("Array access has no index expression");
            return false;
        }

        if (!generateAddress(arrayAccess->array))
            return false;

        if (auto *charIndex = dynamic_cast<CharNode *>(arrayAccess->index)) {
            emit("LIT", 0, (int)charIndex->value);
        }
        else {
            generateExpression(arrayAccess->index);
        }

        emit("CHK", arrayAccess->lowBound, arrayAccess->highBound);

        if (arrayAccess->lowBound != 0) {
            emit("LIT", 0, arrayAccess->lowBound);
            emit("OPR", 0, 3);
        }

        if (arrayAccess->elementSize != 1) {
            emit("LIT", 0, arrayAccess->elementSize);
            emit("OPR", 0, 4);
        }

        emit("OPR", 0, 2);
        return true;
    }

    if (auto *recordAccess = dynamic_cast<RecordAccessNode *>(node)) {
        if (!generateAddress(recordAccess->record))
            return false;

        if (recordAccess->fieldOffset != 0) {
            emit("LIT", 0, recordAccess->fieldOffset);
            emit("OPR", 0, 2);
        }
        return true;
    }

    addError("Unsupported address target: " + node->nodeName());
    return false;
}

bool CodeGenerator::storeTopToTarget(ASTNode *node) {
    RuntimeLocation location = runtimeLocation(node);
    if (location.isValid()) {
        emit("STO", location.levelDiff, location.address);
        return true;
    }

    if (dynamic_cast<ArrayAccessNode *>(node) ||
        dynamic_cast<RecordAccessNode *>(node)) {
        if (!generateAddress(node))
            return false;
        emit("STI", 0, 0);
        return true;
    }

    addError("Unsupported assignment target: " + (node ? node->nodeName() : string("<null>")));
    return false;
}

bool CodeGenerator::copyStructuredValue(ASTNode *source, ASTNode *target, int size) {
    if (size <= 0) {
        addError("Invalid structured copy size " + to_string(size));
        return false;
    }

    if (!generateAddress(source)) {
        addError("Structured assignment source is not addressable");
        return false;
    }
    if (!generateAddress(target)) {
        addError("Structured assignment target is not addressable");
        return false;
    }

    emit("CPY", 0, size);
    return true;
}

bool CodeGenerator::generateLoadSlots(ASTNode *node, int size) {
    if (size <= 0) {
        addError("Invalid structured argument size " + to_string(size));
        return false;
    }

    if (size == 1) {
        generateExpression(node);
        return true;
    }

    for (int offset = 0; offset < size; offset++) {
        if (!generateAddress(node))
            return false;
        if (offset != 0) {
            emit("LIT", 0, offset);
            emit("OPR", 0, 2);
        }
        emit("LDI", 0, 0);
    }
    return true;
}

bool CodeGenerator::generateStoreInput(ASTNode *node) {
    int size = storageSize(node);
    if (size <= 1) {
        emit("REA", 0, 0);
        return storeTopToTarget(node);
    }

    for (int offset = 0; offset < size; offset++) {
        emit("REA", 0, 0);
        if (!generateAddress(node))
            return false;
        if (offset != 0) {
            emit("LIT", 0, offset);
            emit("OPR", 0, 2);
        }
        emit("STI", 0, 0);
    }
    return true;
}

bool CodeGenerator::isStructured(const ASTNode *node) const {
    return storageSize(node) > 1;
}

int CodeGenerator::storageSize(const ASTNode *node) const {
    if (!node)
        return 1;
    return max(1, node->storageSize);
}

int CodeGenerator::operationForBinary(const string &op) const {
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

void CodeGenerator::generateSubprogramCall(const string &name, int tabRef, const vector<ASTNode *> &args, bool leavesResult) {
    int entry = subprogramEntry(tabRef, name);
    if (entry < 0) {
        addError((leavesResult ? "Function" : "Procedure") + string(" call '") + name + "' has no generated entry point");
        return;
    }

    int expected = expectedParamCount(tabRef, name);
    if (expected >= 0 && expected != (int)args.size()) {
        addError("Call to '" + name + "' expects " + to_string(expected) +
                 " argument(s), got " + to_string(args.size()));
        return;
    }

    int actualParamSlots = 0;
    for (auto arg : args) {
        int argSize = storageSize(arg);
        if (!generateLoadSlots(arg, argSize))
            return;
        actualParamSlots += argSize;
    }

    int expectedSlots = expectedParamSlotCount(tabRef, name);
    if (expectedSlots >= 0 && expectedSlots != actualParamSlots) {
        addError("Call to '" + name + "' expects " + to_string(expectedSlots) +
                 " argument slot(s), got " + to_string(actualParamSlots));
        return;
    }

    int levelDiff = 0;
    if (symbolTable && tabRef > 0 && tabRef < symbolTable->getTabSize()) {
        levelDiff = currentFrameLevel - symbolTable->getTabEntry(tabRef).lev;
        if (levelDiff < 0)
            levelDiff = 0;
    }

    emit("CAL", levelDiff, formatCallArgument(entry, actualParamSlots));

    if (leavesResult) {
        if (!symbolTable || tabRef <= 0 || tabRef >= symbolTable->getTabSize()) {
            addError("Cannot load result of function '" + name + "'");
            return;
        }

        const TabEntry &entryInfo = symbolTable->getTabEntry(tabRef);
        int resultLevelDiff = currentFrameLevel - entryInfo.lev;
        if (resultLevelDiff < 0) {
            addError("Invalid lexical level for function result '" + name + "'");
            return;
        }

        emit("LOD", resultLevelDiff, 3 + entryInfo.adr);
    }
}

int CodeGenerator::subprogramEntry(int tabRef, const string &name) const {
    auto byRef = subprogramEntries.find(tabRef);
    if (byRef != subprogramEntries.end())
        return byRef->second;

    auto byName = subprogramEntriesByName.find(lower(name));
    if (byName != subprogramEntriesByName.end())
        return byName->second;

    return -1;
}

int CodeGenerator::expectedParamCount(int tabRef, const string &name) const {
    auto byRef = subprogramParamCounts.find(tabRef);
    if (byRef != subprogramParamCounts.end())
        return byRef->second;

    auto byName = subprogramParamCountsByName.find(lower(name));
    if (byName != subprogramParamCountsByName.end())
        return byName->second;

    return -1;
}

int CodeGenerator::expectedParamSlotCount(int tabRef, const string &name) const {
    auto byRef = subprogramParamSlots.find(tabRef);
    if (byRef != subprogramParamSlots.end())
        return byRef->second;

    auto byName = subprogramParamSlotsByName.find(lower(name));
    if (byName != subprogramParamSlotsByName.end())
        return byName->second;

    return -1;
}

string CodeGenerator::formatCallArgument(int entry, int paramCount) const {
    if (paramCount <= 0)
        return to_string(entry);
    return to_string(entry) + ":" + to_string(paramCount);
}

string CodeGenerator::formatReal(double value) const {
    ostringstream oss;
    oss << setprecision(15) << value;
    return oss.str();
}

string CodeGenerator::quoteLiteral(const string &value) const {
    string escaped;
    for (char ch : value) {
        if (ch == '\\' || ch == '\'')
            escaped.push_back('\\');
        escaped.push_back(ch);
    }
    return "'" + escaped + "'";
}

string CodeGenerator::lower(const string &value) const {
    string result = value;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char ch) { return (char)tolower(ch); });
    return result;
}

void CodeGenerator::addError(const string &message) {
    errors.push_back("Code generation error: " + message);
}

void CodeGenerator::print(ostream &out) const {
    out << "=== Intermediate Code ===" << endl;
    for (const auto &instruction : instructions) {
        out << instruction << endl;
    }

    if (!errors.empty()) {
        out << endl;
        out << "=== Code Generation Errors ===" << endl;
        for (const auto &error : errors) {
            out << error << endl;
        }
    }
}
