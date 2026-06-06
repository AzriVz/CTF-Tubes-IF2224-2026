#include "SemanticAnalyzer.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

static string lowerString(const string &value)
{
    string result = value;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char ch) { return (char)tolower(ch); });
    return result;
}

SemanticAnalyzer::SemanticAnalyzer() : currentLine(0) {}

ParseNode *SemanticAnalyzer::findChild(ParseNode *node, const std::string &name) const
{
    for (auto child : node->children)
    {
        if (child && child->name == name)
        {
            return child;
        }
    }
    return nullptr;
}

std::vector<ParseNode *> SemanticAnalyzer::findAllChildren(ParseNode *node, const std::string &name) const
{
    std::vector<ParseNode *> result;
    for (auto child : node->children)
    {
        if (child && child->name == name)
        {
            result.push_back(child);
        }
    }
    return result;
}

ParseNode *SemanticAnalyzer::getChild(ParseNode *node, int index) const
{
    if (index >= 0 && index < (int)node->children.size())
    {
        return node->children[index];
    }
    return nullptr;
}

std::string SemanticAnalyzer::getToken(ParseNode *node) const
{
    if (node)
        return node->token;
    return "";
}

bool SemanticAnalyzer::hasChild(ParseNode *node, const std::string &name) const
{
    return findChild(node, name) != nullptr;
}

void SemanticAnalyzer::addError(const std::string &msg)
{
    std::ostringstream oss;
    if (currentLine > 0)
    {
        oss << "Error (line " << currentLine << "): " << msg;
    }
    else
    {
        oss << "Error: " << msg;
    }
    errors.push_back(oss.str());
}

void SemanticAnalyzer::addWarning(const std::string &msg)
{
    std::ostringstream oss;
    if (currentLine > 0)
    {
        oss << "Warning (line " << currentLine << "): " << msg;
    }
    else
    {
        oss << "Warning: " << msg;
    }
    warnings.push_back(oss.str());
}

std::string SemanticAnalyzer::typeCodeToString(int typeCode) const
{
    if (typeCode <= 0)
        return "unknown";

    const auto &tab = symTable;
    if (typeCode < tab.getTabSize())
    {
        return tab.getTabEntry(typeCode).id;
    }
    return "unknown";
}

ASTNode *SemanticAnalyzer::resolveTypeNode(const std::string &typeName, ASTNode *typeNode) const
{
    if (auto *varType = dynamic_cast<VarNode *>(typeNode))
    {
        auto named = namedTypeNodes.find(lowerString(varType->name));
        if (named != namedTypeNodes.end())
            return named->second;
    }

    auto named = namedTypeNodes.find(lowerString(typeName));
    if (named != namedTypeNodes.end())
        return named->second;

    return typeNode;
}

int SemanticAnalyzer::typeSize(const std::string &typeName, ASTNode *typeNode) const
{
    ASTNode *resolved = resolveTypeNode(typeName, typeNode);
    if (auto *arrayType = dynamic_cast<ArrayTypeNode *>(resolved))
        return std::max(1, arrayType->totalSize);
    if (auto *recordType = dynamic_cast<RecordTypeNode *>(resolved))
        return std::max(1, recordType->totalSize);

    if (auto *varType = dynamic_cast<VarNode *>(typeNode))
    {
        auto named = namedTypeSizes.find(lowerString(varType->name));
        if (named != namedTypeSizes.end())
            return std::max(1, named->second);
    }

    auto named = namedTypeSizes.find(lowerString(typeName));
    if (named != namedTypeSizes.end())
        return std::max(1, named->second);

    return 1;
}

int SemanticAnalyzer::typeRef(ASTNode *typeNode) const
{
    ASTNode *resolved = resolveTypeNode("", typeNode);
    if (auto *arrayType = dynamic_cast<ArrayTypeNode *>(resolved))
        return arrayType->tabRef;
    return 0;
}

int SemanticAnalyzer::ordinalValue(ASTNode *node, bool &ok) const
{
    ok = true;
    if (auto *number = dynamic_cast<NumberNode *>(node))
        return number->value;
    if (auto *ch = dynamic_cast<CharNode *>(node))
        return (int)ch->value;
    if (auto *boolean = dynamic_cast<BooleanNode *>(node))
        return boolean->value ? 1 : 0;

    ok = false;
    return 0;
}

ArrayTypeNode *SemanticAnalyzer::arrayTypeFor(ASTNode *node) const
{
    if (!node)
        return nullptr;

    if (auto *var = dynamic_cast<VarNode *>(node))
    {
        auto found = typeNodeBySymbol.find(var->tabRef);
        if (found != typeNodeBySymbol.end())
            return dynamic_cast<ArrayTypeNode *>(resolveTypeNode(var->type, found->second));
    }
    else if (auto *arrayAccess = dynamic_cast<ArrayAccessNode *>(node))
    {
        return dynamic_cast<ArrayTypeNode *>(resolveTypeNode(arrayAccess->elementTypeName,
                                                            arrayAccess->elementTypeNode));
    }
    else if (auto *recordAccess = dynamic_cast<RecordAccessNode *>(node))
    {
        return dynamic_cast<ArrayTypeNode *>(resolveTypeNode(recordAccess->fieldTypeName,
                                                            recordAccess->fieldTypeNode));
    }

    return nullptr;
}

RecordTypeNode *SemanticAnalyzer::recordTypeFor(ASTNode *node) const
{
    if (!node)
        return nullptr;

    if (auto *var = dynamic_cast<VarNode *>(node))
    {
        auto found = typeNodeBySymbol.find(var->tabRef);
        if (found != typeNodeBySymbol.end())
            return dynamic_cast<RecordTypeNode *>(resolveTypeNode(var->type, found->second));
    }
    else if (auto *arrayAccess = dynamic_cast<ArrayAccessNode *>(node))
    {
        return dynamic_cast<RecordTypeNode *>(resolveTypeNode(arrayAccess->elementTypeName,
                                                             arrayAccess->elementTypeNode));
    }
    else if (auto *recordAccess = dynamic_cast<RecordAccessNode *>(node))
    {
        return dynamic_cast<RecordTypeNode *>(resolveTypeNode(recordAccess->fieldTypeName,
                                                             recordAccess->fieldTypeNode));
    }

    return nullptr;
}

bool SemanticAnalyzer::isNumeric(int typeCode) const
{
    return typeCode == symTable.getTypeInteger() ||
           typeCode == symTable.getTypeReal();
}

bool SemanticAnalyzer::isBoolean(int typeCode) const
{
    return typeCode == symTable.getTypeBoolean();
}

bool SemanticAnalyzer::isOrdinal(int typeCode) const {
    return typeCode == symTable.getTypeInteger() ||
           typeCode == symTable.getTypeChar() ||
           typeCode == symTable.getTypeBoolean();
}

bool SemanticAnalyzer::isCompatible(int typeCode1, int typeCode2) const {
    if (typeCode1 == typeCode2)
        return true;

    if (isNumeric(typeCode1) && isNumeric(typeCode2))
        return true;

    if (isBoolean(typeCode1) && isBoolean(typeCode2))
        return true;

    if (typeCode1 == symTable.getTypeChar() && typeCode2 == symTable.getTypeChar())
        return true;

    if (typeCode1 == symTable.getTypeString() && typeCode2 == symTable.getTypeString())
        return true;

    return false;
}

bool SemanticAnalyzer::isAssignmentCompatible(int targetType, int valueType) const {
    if (targetType == symTable.getTypeReal() && valueType == symTable.getTypeInteger())
        return true;

    if (isCompatible(targetType, valueType))
        return true;

    return false;
}

int SemanticAnalyzer::inferBinOpType(const std::string &op, int leftType, int rightType, int /*line*/) {
    if (op == "+" || op == "-" || op == "*")
    {
        if (isNumeric(leftType) && isNumeric(rightType))
        {

            if (leftType == symTable.getTypeReal() || rightType == symTable.getTypeReal())
            {
                return symTable.getTypeReal();
            }
            return symTable.getTypeInteger();
        }
        addError("Operator '" + op + "' not defined for types '" +
                 typeCodeToString(leftType) + "' and '" + typeCodeToString(rightType) + "'");
        return 0;
    }

    if (op == "/")
    {
        if (isNumeric(leftType) && isNumeric(rightType))
        {
            return symTable.getTypeReal();
        }
        addError("Operator '/' not defined for types '" +
                 typeCodeToString(leftType) + "' and '" + typeCodeToString(rightType) + "'");
        return 0;
    }

    if (op == "div" || op == "mod")
    {
        if (leftType == symTable.getTypeInteger() && rightType == symTable.getTypeInteger())
        {
            return symTable.getTypeInteger();
        }
        addError("Operator '" + op + "' requires Integer operands, got '" +
                 typeCodeToString(leftType) + "' and '" + typeCodeToString(rightType) + "'");
        return 0;
    }

    if (op == "and" || op == "or")
    {
        if (isBoolean(leftType) && isBoolean(rightType))
        {
            return symTable.getTypeBoolean();
        }
        addError("Operator '" + op + "' requires Boolean operands, got '" +
                 typeCodeToString(leftType) + "' and '" + typeCodeToString(rightType) + "'");
        return 0;
    }

    if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" || op == ">=")
    {
        if (isCompatible(leftType, rightType))
        {
            return symTable.getTypeBoolean();
        }
        addError("Cannot compare types '" + typeCodeToString(leftType) +
                 "' and '" + typeCodeToString(rightType) + "' with operator '" + op + "'");
        return 0;
    }

    return 0;
}

int SemanticAnalyzer::inferUnaryOpType(const std::string &op, int operandType, int /*line*/) {
    if (op == "+" || op == "-")
    {
        if (isNumeric(operandType))
        {
            return operandType;
        }
        addError("Unary operator '" + op + "' requires numeric operand, got '" +
                 typeCodeToString(operandType) + "'");
        return 0;
    }
    if (op == "not")
    {
        if (isBoolean(operandType))
        {
            return symTable.getTypeBoolean();
        }
        addError("Operator 'not' requires Boolean operand, got '" +
                 typeCodeToString(operandType) + "'");
        return 0;
    }
    return 0;
}

ASTNode *SemanticAnalyzer::visit(ParseNode *node)
{
    if (!node)
        return nullptr;

    const std::string &name = node->name;

    if (name == "<program>")
        return visitProgram(node);
    if (name == "<program-header>")
        return nullptr;
    if (name == "<declaration-part>")
        return visitDeclarationPart(node);
    if (name == "<const-declaration>")
        return visitConstDeclaration(node);
    if (name == "<type-declaration>")
        return visitTypeDeclaration(node);
    if (name == "<var-declaration>")
        return visitVarDeclaration(node);
    if (name == "<block>")
        return visit(node->children[0]);
    if (name == "<compound-statement>")
        return visitCompoundStatement(node);
    if (name == "<statement-list>")
        return visitStatementList(node);
    if (name == "<empty-statement>")
        return new EmptyNode();
    if (name == "<assignment-statement>")
        return visitAssignmentStatement(node);
    if (name == "<procedure/function-call>")
        return visitProcedureCall(node);
    if (name == "<if-statement>")
        return visitIfStatement(node);
    if (name == "<while-statement>")
        return visitWhileStatement(node);
    if (name == "<for-statement>")
        return visitForStatement(node);
    if (name == "<repeat-statement>")
        return visitRepeatStatement(node);
    if (name == "<case-statement>")
        return visitCaseStatement(node);
    if (name == "<case-block>")
        return visitCaseBlock(node);
    if (name == "<expression>")
        return visitExpression(node);
    if (name == "<simple-expression>")
        return visitSimpleExpression(node);
    if (name == "<term>")
        return visitTerm(node);
    if (name == "<factor>")
        return visitFactor(node);
    if (name == "<variable>")
        return visitVariable(node);
    if (name == "<component-variable>")
        return nullptr;
    if (name == "<type>")
    {
        std::string dummy;
        return visitType(node, dummy);
    }
    if (name == "<array-type>")
    {
        std::string dummy;
        return visitArrayType(node, dummy);
    }
    if (name == "<record-type>")
    {
        std::string dummy;
        return visitRecordType(node, dummy);
    }
    if (name == "<range>")
    {
        std::string dummy;
        return visitRange(node, dummy);
    }
    if (name == "<enumerated>")
    {
        std::string dummy;
        return visitEnumerated(node, dummy);
    }
    if (name == "<constant>")
        return visitConstant(node);
    if (name == "<subprogram-declaration>")
        return visitSubprogramDeclaration(node);
    if (name == "<procedure-declaration>")
        return visitProcedureDeclaration(node);
    if (name == "<function-declaration>")
        return visitFunctionDeclaration(node);
    if (name == "<formal-parameter-list>")
        return nullptr;
    if (name == "<parameter-group>")
        return nullptr;
    if (name == "<parameter-list>")
        return nullptr;
    if (name == "<relational-operator>")
        return nullptr;
    if (name == "<additive-operator>")
        return nullptr;
    if (name == "<multiplicative-operator>")
        return nullptr;
    if (name == "<identifier-list>")
        return nullptr;

    if (!node->children.empty())
    {
        return visit(node->children[0]);
    }

    return nullptr;
}

ASTNode *SemanticAnalyzer::analyze(ParseNode *parseTree)
{
    symTable.init();
    errors.clear();
    warnings.clear();
    namedTypeNodes.clear();
    namedTypeSizes.clear();
    typeNodeBySymbol.clear();
    sizeBySymbol.clear();

    return visit(parseTree);
}

void SemanticAnalyzer::printResults(std::ostream &out, ASTNode *ast)
{
    out << "=== Decorated AST ===" << std::endl;
    if (ast)
    {
        ast->print(out);
    }
    out << std::endl;

    symTable.printAll(out);

    out << "=== Errors ===" << std::endl;
    if (errors.empty())
    {
        out << "No errors found." << std::endl;
    }
    else
    {
        for (const auto &err : errors)
        {
            out << err << std::endl;
        }
    }
    out << std::endl;

    out << "=== Warnings ===" << std::endl;
    if (warnings.empty())
    {
        out << "No warnings." << std::endl;
    }
    else
    {
        for (const auto &w : warnings)
        {
            out << w << std::endl;
        }
    }
}

ASTNode *SemanticAnalyzer::visitProgram(ParseNode *node)
{
    ParseNode *header = getChild(node, 0);
    std::string progName = visitProgramHeader(header);

    int progIdx = symTable.insertTab(progName, "program", 0, 0, 1, 0, 0);
    symTable.setBlockLast(progIdx);

    ASTNode *decls = nullptr;
    ParseNode *declPart = getChild(node, 1);
    if (declPart)
    {
        decls = visitDeclarationPart(declPart);
    }

    ASTNode *body = nullptr;
    ParseNode *compStmt = getChild(node, 2);
    if (compStmt)
    {
        body = visitCompoundStatement(compStmt);
    }

    ASTNode *prog = new ProgramNode(progName, decls, body);
    prog->tabRef = progIdx;
    return prog;
}

std::string SemanticAnalyzer::visitProgramHeader(ParseNode *node)
{
    return getToken(getChild(node, 1));
}

ASTNode *SemanticAnalyzer::visitDeclarationPart(ParseNode *node)
{
    DeclarationListNode *declList = new DeclarationListNode();
    if (!node)
        return declList;

    for (auto child : node->children)
    {
        if (!child)
            continue;
        const std::string &name = child->name;

        if (name == "<const-declaration>")
        {
            ASTNode *c = visitConstDeclaration(child);
            if (c)
                declList->addDeclaration(c);
        }
        else if (name == "<type-declaration>")
        {
            ASTNode *t = visitTypeDeclaration(child);
            if (t)
                declList->addDeclaration(t);
        }
        else if (name == "<var-declaration>")
        {
            ASTNode *v = visitVarDeclaration(child);
            if (v)
                declList->addDeclaration(v);
        }
        else if (name == "<subprogram-declaration>")
        {
            ASTNode *s = visitSubprogramDeclaration(child);
            if (s)
                declList->addDeclaration(s);
        }
    }

    return declList;
}

ASTNode *SemanticAnalyzer::visitConstDeclaration(ParseNode *node)
{
    DeclarationListNode *result = new DeclarationListNode();

    for (size_t i = 1; i < node->children.size(); i++)
    {
        ParseNode *child = node->children[i];
        if (!child)
            continue;

        if (child->name == "ident")
        {
            std::string constName = child->token;

            if (symTable.lookupInScope(constName, symTable.currentLevel()) >= 0)
            {
                addError("Duplicate identifier '" + constName + "' in current scope");
                continue;
            }

            ParseNode *constNode = (i + 2 < node->children.size()) ? node->children[i + 2] : nullptr;
            ASTNode *valNode = nullptr;
            int constType = 0;
            int constAdr = 0;

            if (constNode && constNode->name == "<constant>")
            {
                valNode = visitConstant(constNode);
                if (valNode)
                {
                    if (auto *num = dynamic_cast<NumberNode *>(valNode))
                    {
                        constType = symTable.getTypeInteger();
                        constAdr = num->value;
                    }
                    else if (auto *real = dynamic_cast<RealNode *>(valNode))
                    {
                        constType = symTable.getTypeReal();
                        constAdr = (int)real->value;
                    }
                    else if (auto *ch = dynamic_cast<CharNode *>(valNode))
                    {
                        constType = symTable.getTypeChar();
                        constAdr = (int)ch->value;
                    }
                    else if (dynamic_cast<StringNode *>(valNode))
                    {
                        constType = symTable.getTypeString();
                        constAdr = 0;
                    }
                    else if (auto *b = dynamic_cast<BooleanNode *>(valNode))
                    {
                        constType = symTable.getTypeBoolean();
                        constAdr = b->value ? 1 : 0;
                    }
                }
            }

            int idx = symTable.insertTab(constName, "constant", constType, 0, 1,
                                         symTable.currentLevel(), constAdr);

            ConstDeclNode *cd = new ConstDeclNode(constName, valNode);
            cd->tabRef = idx;
            cd->type = typeCodeToString(constType);
            cd->level = symTable.currentLevel();
            result->addDeclaration(cd);

            i += 2;
        }
    }

    return result;
}

ASTNode *SemanticAnalyzer::visitConstant(ParseNode *node)
{
    if (node->children.empty())
        return nullptr;

    ParseNode *first = node->children[0];
    const std::string &n = first->name;

    if (n == "intcon")
        return new NumberNode(std::stoi(first->token));
    if (n == "realcon")
        return new RealNode(std::stod(first->token));
    if (n == "charcon")
        return new CharNode(first->token.empty() ? ' ' : first->token[0]);
    if (n == "string")
        return new StringNode(first->token);
    if (n == "ident")
    {

        int idx = symTable.lookup(first->token);
        if (idx < 0)
        {
            addError("Undeclared identifier '" + first->token + "' in constant");
            return nullptr;
        }
        const auto &entry = symTable.getTabEntry(idx);
        if (entry.obj == "constant")
        {
            if (entry.type == symTable.getTypeBoolean())
            {
                return new BooleanNode(entry.adr != 0);
            }
            return new NumberNode(entry.adr);
        }
        addError("Identifier '" + first->token + "' is not a constant");
        return nullptr;
    }
    if (n == "plus" || n == "minus")
    {

        if (node->children.size() >= 2)
        {
            ParseNode *valNode = node->children[1];
            if (valNode->name == "intcon")
            {
                int val = std::stoi(valNode->token);
                if (n == "minus")
                    val = -val;
                return new NumberNode(val);
            }
            if (valNode->name == "realcon")
            {
                double val = std::stod(valNode->token);
                if (n == "minus")
                    val = -val;
                return new RealNode(val);
            }
        }
    }

    return nullptr;
}

ASTNode *SemanticAnalyzer::visitTypeDeclaration(ParseNode *node)
{
    DeclarationListNode *result = new DeclarationListNode();

    for (size_t i = 1; i < node->children.size(); i++)
    {
        if (!node->children[i])
            continue;
        if (node->children[i]->name == "ident")
        {
            std::string typeName = node->children[i]->token;

            if (symTable.lookupInScope(typeName, symTable.currentLevel()) >= 0)
            {
                addError("Duplicate identifier '" + typeName + "' in current scope");
                continue;
            }

            ParseNode *typeSpec = nullptr;
            for (size_t j = i + 1; j < node->children.size(); j++)
            {
                if (node->children[j] && node->children[j]->name == "<type>")
                {
                    typeSpec = node->children[j];
                    break;
                }
            }

            std::string actualType;
            ASTNode *typeNode = nullptr;
            if (typeSpec)
            {
                typeNode = visitType(typeSpec, actualType);
            }

            int typeCode = symTable.getTypeCode(actualType);
            if (typeCode < 0)
            {
                typeCode = symTable.getTabSize();
            }

            ASTNode *resolvedType = resolveTypeNode(actualType, typeNode);
            int size = typeSize(actualType, typeNode);
            int idx = symTable.insertTab(typeName, "type", typeCode, typeRef(resolvedType), 1,
                                         symTable.currentLevel(), 0);
            namedTypeNodes[lowerString(typeName)] = resolvedType;
            namedTypeSizes[lowerString(typeName)] = size;
            typeNodeBySymbol[idx] = resolvedType;
            sizeBySymbol[idx] = size;

            TypeDeclNode *td = new TypeDeclNode(typeName, typeNode);
            td->tabRef = idx;
            td->type = actualType;
            td->level = symTable.currentLevel();
            td->storageSize = size;
            result->addDeclaration(td);
        }
    }

    return result;
}

ASTNode *SemanticAnalyzer::visitType(ParseNode *node, std::string &outTypeName)
{
    if (node->children.empty())
        return nullptr;

    ParseNode *typeChild = node->children[0];
    if (!typeChild)
        return nullptr;

    if (typeChild->name == "ident")
    {

        outTypeName = typeChild->token;

        int idx = symTable.lookup(outTypeName);
        if (idx < 0)
        {
            addError("Undeclared type '" + outTypeName + "'");
            outTypeName = "unknown";
        }
        else
        {
            int typeCode = symTable.getTypeCode(outTypeName);
            if (typeCode > 0)
                outTypeName = typeCodeToString(typeCode);
        }
        return new VarNode(outTypeName);
    }

    if (typeChild->name == "<array-type>")
    {
        return visitArrayType(typeChild, outTypeName);
    }

    if (typeChild->name == "<record-type>")
    {
        return visitRecordType(typeChild, outTypeName);
    }

    if (typeChild->name == "<range>")
    {
        return visitRange(typeChild, outTypeName);
    }

    if (typeChild->name == "<enumerated>")
    {
        return visitEnumerated(typeChild, outTypeName);
    }

    return nullptr;
}

ASTNode *SemanticAnalyzer::visitArrayType(ParseNode *node, std::string &outTypeName)
{
    ParseNode *rangeChild = findChild(node, "<range>");
    ParseNode *elemTypeChild = (node->children.size() >= 6) ? node->children[5] : nullptr;

    std::string indexTypeName;
    ASTNode *indexTypeNode = nullptr;
    if (rangeChild)
    {
        indexTypeNode = visitRange(rangeChild, indexTypeName);
    }

    std::string elemTypeName;
    ASTNode *elemTypeNode = nullptr;
    if (elemTypeChild && elemTypeChild->name == "<type>")
    {
        elemTypeNode = visitType(elemTypeChild, elemTypeName);
    }

    outTypeName = "array[" + indexTypeName + "] of " + elemTypeName;

    int idxTypeCode = symTable.getTypeCode(indexTypeName);
    if (idxTypeCode < 0)
        idxTypeCode = 0;
    int elemTypeCode = symTable.getTypeCode(elemTypeName);
    if (elemTypeCode < 0)
        elemTypeCode = 0;

    if (idxTypeCode == symTable.getTypeReal())
    {
        addError("Array index type cannot be Real");
    }

    int lowBound = 0;
    int highBound = 0;
    if (auto *range = dynamic_cast<RangeNode *>(indexTypeNode))
    {
        if (range->hasBounds)
        {
            lowBound = range->lowValue;
            highBound = range->highValue;
        }
    }

    int elementSize = typeSize(elemTypeName, elemTypeNode);
    int elementCount = std::max(0, highBound - lowBound + 1);
    int totalSize = std::max(1, elementCount * elementSize);
    int atabIdx = symTable.insertAtab(idxTypeCode, elemTypeCode, typeRef(elemTypeNode),
                                      lowBound, highBound, elementSize, totalSize);

    ArrayTypeNode *arrType = new ArrayTypeNode(indexTypeNode, elemTypeNode);
    arrType->tabRef = atabIdx;
    arrType->type = outTypeName;
    arrType->lowBound = lowBound;
    arrType->highBound = highBound;
    arrType->elementSize = elementSize;
    arrType->totalSize = totalSize;
    arrType->elementTypeName = elemTypeName;
    arrType->storageSize = totalSize;
    return arrType;
}

ASTNode *SemanticAnalyzer::visitRecordType(ParseNode *node, std::string &outTypeName)
{
    ParseNode *fieldList = findChild(node, "<field-list>");
    std::vector<VarDeclNode *> fields;
    int offset = 0;

    if (fieldList)
    {

        for (auto child : fieldList->children)
        {
            if (child && child->name == "<field-part>")
            {

                ParseNode *idList = findChild(child, "<identifier-list>");
                ParseNode *typeSpec = nullptr;
                for (auto c : child->children)
                {
                    if (c && c->name == "<type>")
                    {
                        typeSpec = c;
                        break;
                    }
                }

                std::vector<std::string> names;
                if (idList)
                    names = visitIdentifierList(idList);

                std::string fieldTypeName;
                ASTNode *fieldTypeNode = nullptr;
                if (typeSpec)
                {
                    fieldTypeNode = visitType(typeSpec, fieldTypeName);
                }

                for (const auto &fname : names)
                {
                    ASTNode *resolvedFieldType = resolveTypeNode(fieldTypeName, fieldTypeNode);
                    int fieldSize = typeSize(fieldTypeName, fieldTypeNode);
                    VarDeclNode *fd = new VarDeclNode(fname, fieldTypeName, fieldTypeNode);
                    fd->level = symTable.currentLevel();
                    fd->type = fieldTypeName;
                    fd->typeNode = resolvedFieldType;
                    fd->offset = offset;
                    fd->size = fieldSize;
                    fd->storageSize = fieldSize;
                    offset += fieldSize;
                    fields.push_back(fd);
                }
            }
        }
    }

    outTypeName = "record";
    RecordTypeNode *recType = new RecordTypeNode(fields);
    recType->type = outTypeName;
    recType->totalSize = std::max(1, offset);
    recType->storageSize = recType->totalSize;
    return recType;
}

ASTNode *SemanticAnalyzer::visitRange(ParseNode *node, std::string &outTypeName)
{
    ParseNode *low = (node->children.size() >= 1) ? node->children[0] : nullptr;
    ParseNode *high = (node->children.size() >= 4) ? node->children[3] : nullptr;

    ASTNode *lowNode = low ? visitConstant(low) : nullptr;
    ASTNode *highNode = high ? visitConstant(high) : nullptr;

    int lowType = 0, highType = 0;
    if (auto *n = dynamic_cast<NumberNode *>(lowNode))
    {
        lowType = symTable.getTypeInteger();
        if (auto *n2 = dynamic_cast<NumberNode *>(highNode))
        {
            highType = symTable.getTypeInteger();
            if (n->value > n2->value)
            {
                addError("Lower bound of range is greater than upper bound");
            }
        }
    }
    else if (dynamic_cast<CharNode *>(lowNode))
    {
        lowType = symTable.getTypeChar();
        if (dynamic_cast<CharNode *>(highNode))
        {
            highType = symTable.getTypeChar();
        }
    }

    if (lowType > 0 && highType > 0)
    {
        outTypeName = typeCodeToString(lowType);
    }
    else
    {
        outTypeName = "unknown";
    }

    if (lowType == symTable.getTypeReal() || highType == symTable.getTypeReal())
    {
        addError("Subrange cannot have Real type");
        outTypeName = "unknown";
    }

    RangeNode *range = new RangeNode(lowNode, highNode);
    range->type = outTypeName;
    bool lowOk = false;
    bool highOk = false;
    range->lowValue = ordinalValue(lowNode, lowOk);
    range->highValue = ordinalValue(highNode, highOk);
    range->hasBounds = lowOk && highOk;
    return range;
}

ASTNode *SemanticAnalyzer::visitEnumerated(ParseNode *node, std::string &outTypeName)
{
    std::vector<std::string> values;

    for (auto child : node->children)
    {
        if (child && child->name == "ident")
        {
            values.push_back(child->token);
        }
    }

    outTypeName = "enumerated";
    for (size_t i = 0; i < values.size(); i++)
    {
        if (i > 0)
            outTypeName += "_";
        outTypeName += values[i];
    }

    int enumTypeCode = symTable.getTabSize();
    for (size_t i = 0; i < values.size(); i++)
    {
        if (symTable.lookupInScope(values[i], symTable.currentLevel()) < 0)
        {
            symTable.insertTab(values[i], "constant", enumTypeCode, 0, 1,
                               symTable.currentLevel(), (int)i);
        }
    }

    return new EnumeratedNode(values);
}

ASTNode *SemanticAnalyzer::visitVarDeclaration(ParseNode *node)
{
    DeclarationListNode *result = new DeclarationListNode();

    size_t i = 1;
    while (i < node->children.size())
    {
        ParseNode *child = node->children[i];
        if (!child)
        {
            i++;
            continue;
        }

        if (child->name == "<identifier-list>")
        {
            std::vector<std::string> names = visitIdentifierList(child);

            ParseNode *typeSpec = nullptr;
            for (size_t j = i + 1; j < node->children.size(); j++)
            {
                if (!node->children[j])
                    continue;
                if (node->children[j]->name == "<type>")
                {
                    typeSpec = node->children[j];
                    break;
                }
            }

            std::string typeName;
            ASTNode *typeNode = nullptr;
            if (typeSpec)
            {
                typeNode = visitType(typeSpec, typeName);
            }

            int typeCode = symTable.getTypeCode(typeName);
            if (typeCode < 0)
            {
                typeCode = 0;
            }

            ASTNode *resolvedType = resolveTypeNode(typeName, typeNode);
            int storageSize = typeSize(typeName, typeNode);
            int ref = typeRef(resolvedType);

            for (const auto &varName : names)
            {

                int existing = symTable.lookupInScope(varName, symTable.currentLevel());
                if (existing >= 0)
                {
                    addError("Duplicate identifier '" + varName + "' in current scope");
                    continue;
                }

                int adr = symTable.getCurrentParamSize() + symTable.getCurrentVarSize();
                int idx = symTable.insertTab(varName, "variable", typeCode, ref, 1,
                                             symTable.currentLevel(), adr);
                symTable.addVarSize(storageSize);
                typeNodeBySymbol[idx] = resolvedType;
                sizeBySymbol[idx] = storageSize;

                VarDeclNode *vd = new VarDeclNode(varName, typeName, typeNode);
                vd->tabRef = idx;
                vd->type = typeName;
                vd->level = symTable.currentLevel();
                vd->size = storageSize;
                vd->storageSize = storageSize;
                result->addDeclaration(vd);
            }
        }

        i++;
    }

    return result;
}

std::vector<std::string> SemanticAnalyzer::visitIdentifierList(ParseNode *node)
{
    std::vector<std::string> names;
    if (!node)
        return names;

    for (auto child : node->children)
    {
        if (child && child->name == "ident")
        {
            names.push_back(child->token);
        }
    }

    return names;
}

ASTNode *SemanticAnalyzer::visitCompoundStatement(ParseNode *node)
{
    ParseNode *stmtList = findChild(node, "<statement-list>");
    ASTNode *stmts = stmtList ? visitStatementList(stmtList) : nullptr;

    BlockNode *block = new BlockNode(stmts);
    block->level = symTable.currentLevel();
    block->blockIndex = symTable.currentBlock();
    return block;
}

ASTNode *SemanticAnalyzer::visitStatementList(ParseNode *node)
{
    DeclarationListNode *list = new DeclarationListNode();
    if (!node)
        return list;

    for (auto child : node->children)
    {
        if (!child)
            continue;

        if (child->name == "semicolon")
            continue;

        ASTNode *stmt = visit(child);
        if (stmt)
        {
            list->addDeclaration(stmt);
        }
    }

    return list;
}

ASTNode *SemanticAnalyzer::visitAssignmentStatement(ParseNode *node)
{
    ParseNode *targetNode = getChild(node, 0);
    ASTNode *target = nullptr;

    if (targetNode && targetNode->name == "<variable>")
    {
        target = visitVariable(targetNode);
    }
    else if (targetNode && targetNode->name == "ident")
    {

        std::string varName = targetNode->token;
        int idx = symTable.lookup(varName);
        if (idx < 0)
        {
            addError("Undeclared identifier '" + varName + "'");
            target = new VarNode(varName);
        }
        else
        {
            target = new VarNode(varName);
            target->tabRef = idx;
            target->type = typeCodeToString(symTable.getTabEntry(idx).type);
            target->level = symTable.getTabEntry(idx).lev;
            auto sizeIt = sizeBySymbol.find(idx);
            target->storageSize = sizeIt != sizeBySymbol.end() ? sizeIt->second : 1;
        }
    }

    ParseNode *exprNode = getChild(node, 2);
    ASTNode *value = exprNode ? visitExpression(exprNode) : nullptr;

    if (target && value)
    {
        int targetType = symTable.lookup(target->type);
        int valueType = symTable.lookup(value->type);

        if (targetType < 0)
            targetType = 0;
        if (valueType < 0)
            valueType = 0;

        if (targetType > 0 && valueType > 0)
        {
            if (!isAssignmentCompatible(targetType, valueType))
            {
                addError("Assignment incompatible: cannot assign value of type '" +
                         value->type + "' to variable of type '" + target->type + "'");
            }
        }
    }

    AssignNode *assign = new AssignNode(target, value);
    assign->level = symTable.currentLevel();
    assign->type = "void";
    return assign;
}

ASTNode *SemanticAnalyzer::visitProcedureCall(ParseNode *node)
{
    ParseNode *nameNode = getChild(node, 0);
    std::string callName = nameNode ? nameNode->token : "";
    std::string lowerCallName = lowerString(callName);

    int idx = symTable.lookup(callName);
    bool isFunction = false;
    if (idx < 0)
    {
        if (lowerCallName != "write")
        {
            addError("Undeclared identifier '" + callName + "'");
        }
    }
    else
    {
        const auto &entry = symTable.getTabEntry(idx);
        if (entry.obj == "function")
        {
            isFunction = true;
        }
        else if (entry.obj != "procedure")
        {
            addError("'" + callName + "' is not a procedure or function");
        }
    }

    // Process parameters
    std::vector<ASTNode *> args;
    ParseNode *paramList = findChild(node, "<parameter-list>");
    if (paramList)
    {
        for (auto child : paramList->children)
        {
            if (child && (child->name == "<expression>" || child->name == "<simple-expression>" || child->name == "<term>" || child->name == "<factor>"))
            {
                ASTNode *arg = visitExpression(child);
                if (arg)
                    args.push_back(arg);
            }
            else if (child && child->name == "comma")
            {
            }
            else if (child)
            {
                ASTNode *arg = visit(child);
                if (arg)
                    args.push_back(arg);
            }
        }
    }

    if (isFunction)
    {
        FuncCallNode *call = new FuncCallNode(callName, args);
        call->tabRef = (idx >= 0) ? idx : 0;
        if (idx >= 0)
        {
            call->type = typeCodeToString(symTable.getTabEntry(idx).type);
        }
        return call;
    }

    ProcCallNode *call = new ProcCallNode(callName, args);
    call->tabRef = (idx >= 0) ? idx : 0;
    return call;
}

ASTNode *SemanticAnalyzer::visitIfStatement(ParseNode *node)
{
    ParseNode *exprNode = findChild(node, "<expression>");
    ASTNode *condition = exprNode ? visitExpression(exprNode) : nullptr;

    ASTNode *thenStmt = nullptr;
    ASTNode *elseStmt = nullptr;
    bool foundThen = false;
    bool foundElse = false;

    for (auto child : node->children)
    {
        if (!child)
            continue;
        if (child->name == "ifsy" || child->name == "<expression>")
        {
            continue;
        }
        if (child->name == "thensy")
        {
            foundThen = true;
            foundElse = false;
            continue;
        }
        if (child->name == "elsesy")
        {
            foundThen = false;
            foundElse = true;
            continue;
        }
        if (foundThen && !thenStmt)
        {
            thenStmt = visit(child);
            foundThen = false;
        }
        else if (foundElse && !elseStmt)
        {
            elseStmt = visit(child);
        }
    }

    if (condition && !condition->type.empty())
    {
        int condType = symTable.getTypeCode(condition->type);
        if (!isBoolean(condType))
        {
            addError("If condition must be of type 'boolean', got '" +
                     condition->type + "'");
        }
    }

    IfNode *ifn = new IfNode(condition, thenStmt, elseStmt);
    ifn->level = symTable.currentLevel();
    return ifn;
}

ASTNode *SemanticAnalyzer::visitWhileStatement(ParseNode *node)
{
    ParseNode *exprNode = findChild(node, "<expression>");
    ASTNode *condition = exprNode ? visitExpression(exprNode) : nullptr;

    ParseNode *compStmt = findChild(node, "<compound-statement>");
    ASTNode *body = compStmt ? visitCompoundStatement(compStmt) : nullptr;

    if (condition && !condition->type.empty())
    {
        int condType = symTable.getTypeCode(condition->type);
        if (!isBoolean(condType))
        {
            addError("While condition must be of type 'boolean', got '" +
                     condition->type + "'");
        }
    }

    WhileNode *wn = new WhileNode(condition, body);
    wn->level = symTable.currentLevel();
    return wn;
}

ASTNode *SemanticAnalyzer::visitForStatement(ParseNode *node)
{
    std::string loopVar;
    std::string direction = "to";
    ASTNode *start = nullptr;
    ASTNode *end = nullptr;
    ASTNode *body = nullptr;

    for (size_t i = 0; i < node->children.size(); i++)
    {
        auto child = node->children[i];
        if (!child)
            continue;

        if (child->name == "ident")
        {
            loopVar = child->token;
        }
        else if (child->name == "tosy")
        {
            direction = "to";
        }
        else if (child->name == "downtosy")
        {
            direction = "downto";
        }
    }

    for (size_t i = 0; i < node->children.size(); i++)
    {
        auto child = node->children[i];
        if (!child)
            continue;
        if (child->name == "<expression>")
        {
            if (!start)
            {
                start = visitExpression(child);
            }
            else if (!end)
            {
                end = visitExpression(child);
            }
        }
    }

    ParseNode *compStmt = findChild(node, "<compound-statement>");
    if (compStmt)
        body = visitCompoundStatement(compStmt);

    int varIdx = symTable.lookup(loopVar);
    if (varIdx < 0)
    {
        addError("Undeclared identifier '" + loopVar + "' in for loop");
    }

    ForNode *fn = new ForNode(loopVar, direction, start, end, body);
    fn->level = symTable.currentLevel();
    return fn;
}

ASTNode *SemanticAnalyzer::visitRepeatStatement(ParseNode *node)
{
    ParseNode *stmtList = findChild(node, "<statement-list>");
    ASTNode *body = stmtList ? visitStatementList(stmtList) : nullptr;

    ParseNode *exprNode = nullptr;
    for (auto child : node->children)
    {
        if (child && child->name == "<expression>")
        {
            exprNode = child;
            break;
        }
    }
    ASTNode *condition = exprNode ? visitExpression(exprNode) : nullptr;

    if (condition && !condition->type.empty())
    {
        int condType = symTable.getTypeCode(condition->type);
        if (!isBoolean(condType))
        {
            addError("Repeat-until condition must be of type 'boolean', got '" +
                     condition->type + "'");
        }
    }

    RepeatNode *rn = new RepeatNode(body, condition);
    rn->level = symTable.currentLevel();
    return rn;
}

ASTNode *SemanticAnalyzer::visitCaseStatement(ParseNode *node)
{
    ParseNode *selectorNode = findChild(node, "<expression>");
    ASTNode *selector = selectorNode ? visitExpression(selectorNode) : nullptr;

    std::vector<CaseBranchNode *> branches;
    for (auto child : node->children)
    {
        if (child && child->name == "<case-block>")
        {
            CaseBranchNode *branch = dynamic_cast<CaseBranchNode *>(visitCaseBlock(child));
            if (branch)
                branches.push_back(branch);
        }
    }

    CaseNode *cn = new CaseNode(selector, branches);
    cn->level = symTable.currentLevel();
    return cn;
}

ASTNode *SemanticAnalyzer::visitCaseBlock(ParseNode *node)
{
    std::vector<ASTNode *> labels;
    ASTNode *stmt = nullptr;

    for (auto child : node->children)
    {
        if (!child)
            continue;
        if (child->name == "<constant>")
        {
            labels.push_back(visitConstant(child));
        }
        else if (child->name != "comma" && child->name != "colon" && child->name != "semicolon")
        {
            stmt = visit(child);
        }
    }

    return new CaseBranchNode(labels, stmt);
}

ASTNode *SemanticAnalyzer::visitExpression(ParseNode *node)
{
    if (node->children.empty())
        return nullptr;

    ParseNode *leftExpr = node->children[0];
    ASTNode *left = leftExpr ? visitSimpleExpression(leftExpr) : nullptr;

    ParseNode *relOp = findChild(node, "<relational-operator>");
    if (relOp && node->children.size() >= 3)
    {
        std::string op = visitRelationalOperator(relOp);
        ParseNode *rightExpr = node->children[2];
        ASTNode *right = rightExpr ? visitSimpleExpression(rightExpr) : nullptr;

        int leftType = symTable.lookup(left ? left->type : "");
        int rightType = symTable.lookup(right ? right->type : "");

        BinOpNode *binOp = new BinOpNode(op, left, right);
        binOp->level = symTable.currentLevel();

        int resultType = inferBinOpType(op, leftType, rightType, currentLine);
        binOp->type = typeCodeToString(resultType);
        if (left)
            binOp->type = "Boolean";

        return binOp;
    }

    return left;
}

ASTNode *SemanticAnalyzer::visitSimpleExpression(ParseNode *node)
{
    if (node->children.empty())
        return nullptr;

    size_t pos = 0;
    ASTNode *result = nullptr;

    std::string unaryOp;
    if (node->children[0] && (node->children[0]->name == "plus" || node->children[0]->name == "minus"))
    {
        unaryOp = node->children[0]->name == "plus" ? "+" : "-";
        pos = 1;
    }

    if (pos < node->children.size() && node->children[pos])
    {
        result = visitTerm(node->children[pos]);
        pos++;
    }

    if (!unaryOp.empty() && result)
    {
        int operandType = symTable.lookup(result->type);
        int resType = inferUnaryOpType(unaryOp, operandType, currentLine);
        UnaryOpNode *uop = new UnaryOpNode(unaryOp, result);
        uop->type = typeCodeToString(resType);
        uop->level = symTable.currentLevel();
        result = uop;
    }

    while (pos < node->children.size())
    {
        ParseNode *opNode = node->children[pos];
        if (!opNode || opNode->name != "<additive-operator>")
        {
            pos++;
            continue;
        }

        std::string op = visitAdditiveOperator(opNode);
        pos++;

        if (pos < node->children.size() && node->children[pos])
        {
            ASTNode *right = visitTerm(node->children[pos]);
            pos++;

            if (result && right)
            {
                int leftType = symTable.lookup(result->type);
                int rightType = symTable.lookup(right->type);
                int resType = inferBinOpType(op, leftType, rightType, currentLine);

                BinOpNode *binOp = new BinOpNode(op, result, right);
                binOp->type = typeCodeToString(resType);
                binOp->level = symTable.currentLevel();
                result = binOp;
            }
        }
    }

    return result;
}

ASTNode *SemanticAnalyzer::visitTerm(ParseNode *node)
{
    if (node->children.empty())
        return nullptr;

    ASTNode *result = nullptr;
    size_t pos = 0;

    if (pos < node->children.size() && node->children[pos])
    {
        result = visitFactor(node->children[pos]);
        pos++;
    }

    while (pos < node->children.size())
    {
        ParseNode *opNode = node->children[pos];
        if (!opNode || opNode->name != "<multiplicative-operator>")
        {
            pos++;
            continue;
        }

        std::string op = visitMultiplicativeOperator(opNode);
        pos++;

        if (pos < node->children.size() && node->children[pos])
        {
            ASTNode *right = visitFactor(node->children[pos]);
            pos++;

            if (result && right)
            {
                int leftType = symTable.lookup(result->type);
                int rightType = symTable.lookup(right->type);
                int resType = inferBinOpType(op, leftType, rightType, currentLine);

                BinOpNode *binOp = new BinOpNode(op, result, right);
                binOp->type = typeCodeToString(resType);
                binOp->level = symTable.currentLevel();
                result = binOp;
            }
        }
    }

    return result;
}

ASTNode *SemanticAnalyzer::visitFactor(ParseNode *node)
{
    if (node->children.empty())
        return nullptr;

    ParseNode *first = node->children[0];
    if (!first)
        return nullptr;

    const std::string &n = first->name;

    if (n == "intcon")
    {
        NumberNode *num = new NumberNode(std::stoi(first->token));
        num->level = symTable.currentLevel();
        return num;
    }
    if (n == "realcon")
    {
        RealNode *real = new RealNode(std::stod(first->token));
        real->level = symTable.currentLevel();
        return real;
    }
    if (n == "string")
    {
        StringNode *str = new StringNode(first->token);
        str->level = symTable.currentLevel();
        return str;
    }
    if (n == "charcon")
    {
        CharNode *ch = new CharNode(first->token.empty() ? ' ' : first->token[0]);
        ch->level = symTable.currentLevel();
        return ch;
    }

    if (n == "notsy")
    {

        ASTNode *operand = (node->children.size() >= 2) ? visitFactor(node->children[1]) : nullptr;
        if (operand)
        {
            int opType = symTable.lookup(operand->type);
            int resType = inferUnaryOpType("not", opType, currentLine);
            UnaryOpNode *uop = new UnaryOpNode("not", operand);
            uop->type = typeCodeToString(resType);
            uop->level = symTable.currentLevel();
            return uop;
        }
        return nullptr;
    }

    if (n == "lparent")
    {

        for (auto child : node->children)
        {
            if (child && child->name == "<expression>")
            {
                return visitExpression(child);
            }
        }
        return nullptr;
    }

    if (n == "ident" || n == "<variable>" || n == "<procedure/function-call>")
    {
        if (n == "ident")
        {

            std::string idName = first->token;

            if (node->children.size() >= 2 && node->children[1] &&
                node->children[1]->name == "<procedure/function-call>")
            {
                return visitProcedureCall(node->children[1]);
            }

            if (node->children.size() >= 2)
            {

                for (auto child : node->children)
                {
                    if (child && child->name == "<variable>")
                    {
                        return visitVariable(child);
                    }
                }
            }

            int idx = symTable.lookup(idName);
            if (idx < 0)
            {
                addError("Undeclared identifier '" + idName + "'");
                VarNode *v = new VarNode(idName);
                v->level = symTable.currentLevel();
                return v;
            }

            const auto &entry = symTable.getTabEntry(idx);

            if (entry.obj == "constant")
            {
                if (entry.type == symTable.getTypeBoolean())
                {
                    BooleanNode *b = new BooleanNode(entry.adr != 0);
                    b->level = symTable.currentLevel();
                    return b;
                }
                if (entry.type == symTable.getTypeInteger())
                {
                    NumberNode *num = new NumberNode(entry.adr);
                    num->level = symTable.currentLevel();
                    return num;
                }
            }

            VarNode *v = new VarNode(idName);
            v->tabRef = idx;
            v->type = typeCodeToString(entry.type);
            v->level = entry.lev;
            auto sizeIt = sizeBySymbol.find(idx);
            v->storageSize = sizeIt != sizeBySymbol.end() ? sizeIt->second : 1;
            return v;
        }

        if (n == "<variable>")
        {
            return visitVariable(first);
        }

        if (n == "<procedure/function-call>")
        {
            return visitProcedureCall(first);
        }
    }

    return nullptr;
}

ASTNode *SemanticAnalyzer::visitVariable(ParseNode *node)
{
    if (node->children.empty())
        return nullptr;

    ParseNode *idNode = node->children[0];
    if (!idNode || idNode->name != "ident")
        return nullptr;

    std::string varName = idNode->token;
    int idx = symTable.lookup(varName);
    if (idx < 0)
    {
        addError("Undeclared identifier '" + varName + "'");
        VarNode *v = new VarNode(varName);
        v->level = symTable.currentLevel();
        return v;
    }

    const auto &entry = symTable.getTabEntry(idx);
    int currentType = entry.type;

    ASTNode *result = new VarNode(varName);
    result->tabRef = idx;
    result->type = typeCodeToString(currentType);
    result->level = entry.lev;
    auto sizeIt = sizeBySymbol.find(idx);
    result->storageSize = sizeIt != sizeBySymbol.end() ? sizeIt->second : 1;

    for (size_t i = 1; i < node->children.size(); i++)
    {
        if (node->children[i] && node->children[i]->name == "<component-variable>")
        {
            result = visitComponentVariable(node->children[i], result);
        }
    }

    return result;
}

ASTNode *SemanticAnalyzer::visitComponentVariable(ParseNode *node, ASTNode *base)
{
    if (!node || node->children.empty())
        return base;

    ParseNode *first = node->children[0];
    if (!first)
        return base;

    if (first->name == "lbrack")
    {

        ParseNode *indexList = findChild(node, "<index-list>");
        std::vector<ASTNode *> indexes;
        if (indexList)
        {

            for (auto child : indexList->children)
            {
                if (child && (child->name == "intcon" || child->name == "ident" || child->name == "charcon"))
                {
                    ASTNode *index = nullptr;
                    if (child->name == "intcon")
                    {
                        index = new NumberNode(std::stoi(child->token));
                        index->level = symTable.currentLevel();
                    }
                    else if (child->name == "ident")
                    {
                        int idx = symTable.lookup(child->token);
                        if (idx < 0)
                        {
                            addError("Undeclared identifier '" + child->token + "'");
                        }
                        index = new VarNode(child->token);
                        if (idx >= 0)
                        {
                            index->tabRef = idx;
                            index->type = typeCodeToString(symTable.getTabEntry(idx).type);
                            index->level = symTable.getTabEntry(idx).lev;
                            auto sizeIt = sizeBySymbol.find(idx);
                            index->storageSize = sizeIt != sizeBySymbol.end() ? sizeIt->second : 1;
                        }
                    }
                    else if (child->name == "charcon")
                    {
                        index = new CharNode(child->token.empty() ? ' ' : child->token[0]);
                        index->level = symTable.currentLevel();
                    }
                    if (index)
                        indexes.push_back(index);
                }
            }
        }

        if (indexes.empty())
        {
            addError("Array access requires at least one index");
            return base;
        }

        ASTNode *result = base;
        for (auto index : indexes)
        {
            ArrayAccessNode *arr = new ArrayAccessNode(result, index);
            arr->level = symTable.currentLevel();
            ArrayTypeNode *arrayType = arrayTypeFor(result);
            if (!arrayType)
            {
                addError("Indexed access used on non-array value");
                arr->type = "unknown";
                result = arr;
                continue;
            }

            arr->lowBound = arrayType->lowBound;
            arr->highBound = arrayType->highBound;
            arr->elementSize = arrayType->elementSize;
            arr->totalSize = arrayType->totalSize;
            arr->elementTypeName = arrayType->elementTypeName;
            arr->elementTypeNode = resolveTypeNode(arrayType->elementTypeName, arrayType->elementType);
            arr->type = arrayType->elementTypeName;
            arr->tabRef = result ? result->tabRef : 0;
            arr->storageSize = std::max(1, arrayType->elementSize);
            result = arr;
        }
        return result;
    }

    if (first->name == "period")
    {

        std::string fieldName = (node->children.size() >= 2 && node->children[1])
                                    ? node->children[1]->token
                                    : "";
        RecordAccessNode *rec = new RecordAccessNode(base, fieldName);
        rec->level = symTable.currentLevel();
        RecordTypeNode *recordType = recordTypeFor(base);
        if (!recordType)
        {
            addError("Field access used on non-record value");
            rec->type = "unknown";
        }
        else
        {
            bool found = false;
            for (auto field : recordType->fields)
            {
                if (field && lowerString(field->varName) == lowerString(fieldName))
                {
                    rec->fieldOffset = field->offset;
                    rec->fieldSize = field->size;
                    rec->fieldTypeName = field->typeName;
                    rec->fieldTypeNode = resolveTypeNode(field->typeName, field->typeNode);
                    rec->type = field->typeName;
                    rec->tabRef = base ? base->tabRef : 0;
                    rec->storageSize = std::max(1, field->size);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                addError("Record has no field named '" + fieldName + "'");
                rec->type = "unknown";
            }
        }
        return rec;
    }

    return base;
}

ASTNode *SemanticAnalyzer::visitSubprogramDeclaration(ParseNode *node)
{
    if (node->children.empty())
        return nullptr;
    ParseNode *child = node->children[0];
    if (!child)
        return nullptr;

    if (child->name == "<procedure-declaration>")
    {
        return visitProcedureDeclaration(child);
    }
    if (child->name == "<function-declaration>")
    {
        return visitFunctionDeclaration(child);
    }
    return nullptr;
}

ASTNode *SemanticAnalyzer::visitProcedureDeclaration(ParseNode *node)
{
    std::string procName;
    for (auto child : node->children)
    {
        if (child && child->name == "ident")
        {
            procName = child->token;
            break;
        }
    }

    std::vector<VarDeclNode *> params;
    ParseNode *paramList = findChild(node, "<formal-parameter-list>");
    if (paramList)
    {
        visitFormalParameterList(paramList, params);
    }

    int procIdx = symTable.insertTab(procName, "procedure", 0, 0, 1, symTable.currentLevel(), 0);

    symTable.pushScope();

    for (auto param : params)
    {
        int typeCode = symTable.getTypeCode(param->typeName);
        if (typeCode < 0)
            typeCode = 0;
        ASTNode *resolvedType = resolveTypeNode(param->typeName, param->typeNode);
        int paramSize = typeSize(param->typeName, param->typeNode);
        int adr = symTable.getCurrentParamSize();
        int idx = symTable.insertTab(param->varName, "parameter", typeCode, typeRef(resolvedType), param->level == 1 ? 1 : 0,
                                     symTable.currentLevel(), adr);
        symTable.addParamSize(paramSize);
        param->tabRef = idx;
        param->level = symTable.currentLevel();
        param->typeNode = resolvedType;
        param->size = paramSize;
        param->storageSize = paramSize;
        typeNodeBySymbol[idx] = resolvedType;
        sizeBySymbol[idx] = paramSize;
    }

    ParseNode *blockNode = findChild(node, "<block>");
    ASTNode *body = nullptr;
    if (blockNode)
    {

        ASTNode *decls = nullptr;
        ASTNode *compStmt = nullptr;
        for (auto child : blockNode->children)
        {
            if (child && child->name == "<declaration-part>")
            {
                decls = visitDeclarationPart(child);
            }
            if (child && child->name == "<compound-statement>")
            {

                symTable.pushScope();
                compStmt = visitCompoundStatement(child);
                symTable.popScope();
            }
        }

        BlockNode *block = new BlockNode();
        block->level = symTable.currentLevel();
        block->blockIndex = symTable.currentBlock();
        if (decls)
            block->addChild(decls);
        if (compStmt)
            block->addChild(compStmt);
        body = block;
    }

    symTable.popScope();

    SubprogramDeclNode *sd = new SubprogramDeclNode("procedure", procName, params, "", body);
    sd->tabRef = procIdx;
    sd->level = symTable.currentLevel();
    return sd;
}

ASTNode *SemanticAnalyzer::visitFunctionDeclaration(ParseNode *node)
{
    std::string funcName;
    std::string returnType;
    for (size_t i = 0; i < node->children.size(); i++)
    {
        if (!node->children[i])
            continue;
        if (node->children[i]->name == "ident")
        {
            if (funcName.empty())
            {
                funcName = node->children[i]->token;
            }
        }
        if (node->children[i]->name == "colon")
        {

            if (i + 1 < node->children.size() && node->children[i + 1] && node->children[i + 1]->name == "ident")
            {
                returnType = node->children[i + 1]->token;
            }
        }
    }

    std::vector<VarDeclNode *> params;
    ParseNode *paramList = findChild(node, "<formal-parameter-list>");
    if (paramList)
    {
        visitFormalParameterList(paramList, params);
    }

    int retTypeCode = symTable.getTypeCode(returnType);
    if (retTypeCode < 0)
    {
        addError("Undeclared return type '" + returnType + "' for function '" + funcName + "'");
        retTypeCode = 0;
    }

    int resultAdr = symTable.getCurrentParamSize() + symTable.getCurrentVarSize();
    int funcIdx = symTable.insertTab(funcName, "function", retTypeCode, 0, 1,
                                     symTable.currentLevel(), resultAdr);
    symTable.addVarSize(1);

    symTable.pushScope();

    for (auto param : params)
    {
        int typeCode = symTable.getTypeCode(param->typeName);
        if (typeCode < 0)
            typeCode = 0;
        ASTNode *resolvedType = resolveTypeNode(param->typeName, param->typeNode);
        int paramSize = typeSize(param->typeName, param->typeNode);
        int adr = symTable.getCurrentParamSize();
        int idx = symTable.insertTab(param->varName, "parameter", typeCode, typeRef(resolvedType), 1,
                                     symTable.currentLevel(), adr);
        symTable.addParamSize(paramSize);
        param->tabRef = idx;
        param->level = symTable.currentLevel();
        param->typeNode = resolvedType;
        param->size = paramSize;
        param->storageSize = paramSize;
        typeNodeBySymbol[idx] = resolvedType;
        sizeBySymbol[idx] = paramSize;
    }

    ParseNode *blockNode = findChild(node, "<block>");
    ASTNode *body = nullptr;
    if (blockNode)
    {
        ASTNode *decls = nullptr;
        ASTNode *compStmt = nullptr;
        for (auto child : blockNode->children)
        {
            if (child && child->name == "<declaration-part>")
            {
                decls = visitDeclarationPart(child);
            }
            if (child && child->name == "<compound-statement>")
            {
                symTable.pushScope();
                compStmt = visitCompoundStatement(child);
                symTable.popScope();
            }
        }

        BlockNode *block = new BlockNode();
        block->level = symTable.currentLevel();
        block->blockIndex = symTable.currentBlock();
        if (decls)
            block->addChild(decls);
        if (compStmt)
            block->addChild(compStmt);
        body = block;
    }

    symTable.popScope();

    SubprogramDeclNode *sd = new SubprogramDeclNode("function", funcName, params, returnType, body);
    sd->tabRef = funcIdx;
    sd->level = symTable.currentLevel();
    sd->type = returnType;
    return sd;
}

ASTNode *SemanticAnalyzer::visitFormalParameterList(ParseNode *node, std::vector<VarDeclNode *> &params)
{
    for (auto child : node->children)
    {
        if (child && child->name == "<parameter-group>")
        {
            visitParameterGroup(child, params);
        }
    }
    return nullptr;
}

ASTNode *SemanticAnalyzer::visitParameterGroup(ParseNode *node, std::vector<VarDeclNode *> &params)
{
    ParseNode *idList = findChild(node, "<identifier-list>");
    std::vector<std::string> names = idList ? visitIdentifierList(idList) : std::vector<std::string>();

    ParseNode *typeSpec = findChild(node, "<type>");
    std::string typeName;
    ASTNode *typeNode = nullptr;
    if (typeSpec)
    {
        typeNode = visitType(typeSpec, typeName);
    }

    for (const auto &name : names)
    {
        VarDeclNode *vd = new VarDeclNode(name, typeName, typeNode);
        vd->type = typeName;
        vd->typeNode = resolveTypeNode(typeName, typeNode);
        vd->size = typeSize(typeName, typeNode);
        vd->storageSize = vd->size;
        params.push_back(vd);
    }

    return nullptr;
}

std::string SemanticAnalyzer::visitRelationalOperator(ParseNode *node)
{
    if (!node->children.empty())
    {
        const std::string &name = node->children[0]->name;
        if (name == "eql")
            return "=";
        if (name == "neq")
            return "<>";
        if (name == "gtr")
            return ">";
        if (name == "geq")
            return ">=";
        if (name == "lss")
            return "<";
        if (name == "leq")
            return "<=";
    }
    return "=";
}

std::string SemanticAnalyzer::visitAdditiveOperator(ParseNode *node)
{
    if (!node->children.empty())
    {
        const std::string &name = node->children[0]->name;
        if (name == "plus")
            return "+";
        if (name == "minus")
            return "-";
        if (name == "orsy")
            return "or";
    }
    return "+";
}

std::string SemanticAnalyzer::visitMultiplicativeOperator(ParseNode *node)
{
    if (!node->children.empty())
    {
        const std::string &name = node->children[0]->name;
        if (name == "times")
            return "*";
        if (name == "rdiv")
            return "/";
        if (name == "idiv")
            return "div";
        if (name == "imod")
            return "mod";
        if (name == "andsy")
            return "and";
    }
    return "*";
}
