#include "AST.hpp"
#include <algorithm>

ASTNode::ASTNode()
    : type(""), level(0), tabRef(0), line(0), storageSize(1) {}

ASTNode::~ASTNode() {
    for (auto child : children) {
        delete child;
    }
}

std::string ASTNode::annotation() const {
    return ASTHelper::formatAnnotation(type, level, tabRef);
}

std::string ASTNode::label() const {
    return nodeName();
}

void ASTNode::addChild(ASTNode *child) {
    if (child) {
        children.push_back(child);
    }
}

void ASTNode::print(std::ostream &out, const std::string &prefix, bool isLast) const {
    out << prefix;
    if (prefix.empty()) {
        out << label();
    } else {
        out << (isLast ? "\u2514\u2500\u2500 " : "\u251C\u2500\u2500 ") << label();
    }

    std::string ann = annotation();
    if (!ann.empty()) {
        out << ann;
    }
    out << std::endl;

    std::string childPrefix = prefix + (isLast ? "    " : "\u2502   ");

    for (std::size_t i = 0; i < children.size(); i++) {
        children[i]->print(out, childPrefix, i + 1 == children.size());
    }
}

ProgramNode::ProgramNode(const std::string &n, ASTNode *decls, ASTNode *blk)
    : name(n), declarations(decls), body(blk) {
    if (decls)
        addChild(decls);
    if (blk)
        addChild(blk);
}

std::string ProgramNode::label() const {
    return "ProgramNode(name: '" + name + "')";
}

std::string ProgramNode::annotation() const {
    std::string ann = ASTHelper::formatAnnotation(type, level, tabRef);
    if (!ann.empty())
        return ann;
    return "";
}

BlockNode::BlockNode(ASTNode *stmts) : blockIndex(0) {
    if (stmts)
        addChild(stmts);
}

std::string BlockNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  block_index:" << blockIndex;
    if (level > 0)
        oss << ", lev:" << level;
    if (!type.empty())
        oss << ", type:" << type;
    return oss.str();
}

DeclarationListNode::DeclarationListNode() {}

void DeclarationListNode::addDeclaration(ASTNode *decl) {
    if (decl)
        addChild(decl);
}

VarDeclNode::VarDeclNode(const std::string &name, const std::string &tname, ASTNode *tnode)
    : varName(name), typeName(tname), typeNode(tnode), offset(0), size(1) {}

std::string VarDeclNode::label() const {
    return "VarDecl('" + varName + "')";
}

std::string VarDeclNode::annotation() const {

    std::ostringstream oss;
    oss << "  ->  tab_index:" << tabRef << ", type:" << (typeName.empty() ? type : typeName)
        << ", lev:" << level;
    if (offset != 0)
        oss << ", offset:" << offset;
    if (size != 1)
        oss << ", size:" << size;
    return oss.str();
}

ConstDeclNode::ConstDeclNode(const std::string &name, ASTNode *val)
    : constName(name), value(val) {
    if (val)
        addChild(val);
}

std::string ConstDeclNode::label() const {
    return "ConstDecl('" + constName + "')";
}

std::string ConstDeclNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  tab_index:" << tabRef << ", type:" << type << ", lev:" << level;
    return oss.str();
}

TypeDeclNode::TypeDeclNode(const std::string &name, ASTNode *def)
    : typeName(name), typeDef(def) {
    if (def)
        addChild(def);
}

std::string TypeDeclNode::label() const {
    return "TypeDecl('" + typeName + "')";
}

std::string TypeDeclNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  tab_index:" << tabRef << ", type:" << type << ", lev:" << level;
    return oss.str();
}

AssignNode::AssignNode(ASTNode *tgt, ASTNode *val)
    : target(tgt), value(val) {
    if (tgt)
        addChild(tgt);
    if (val)
        addChild(val);
}

std::string AssignNode::label() const {
    return "Assign";
}

std::string AssignNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:void";
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

ProcCallNode::ProcCallNode(const std::string &name, const std::vector<ASTNode *> &arguments)
    : procName(name), args(arguments) {
    for (auto arg : args) {
        if (arg)
            addChild(arg);
    }
}

std::string ProcCallNode::label() const {
    return "ProcCall(" + procName + ")";
}

std::string ProcCallNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  tab_index:" << tabRef;
    if (!type.empty())
        oss << ", type:" << type;
    return oss.str();
}

IfNode::IfNode(ASTNode *cond, ASTNode *thenS, ASTNode *elseS)
    : condition(cond), thenStmt(thenS), elseStmt(elseS) {
    if (cond)
        addChild(cond);
    if (thenS)
        addChild(thenS);
    if (elseS)
        addChild(elseS);
}

std::string IfNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:void";
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

WhileNode::WhileNode(ASTNode *cond, ASTNode *bdy)
    : condition(cond), body(bdy) {
    if (cond)
        addChild(cond);
    if (bdy)
        addChild(bdy);
}

std::string WhileNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:void";
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

ForNode::ForNode(const std::string &var, const std::string &dir,
                 ASTNode *st, ASTNode *en, ASTNode *bdy)
    : loopVar(var), direction(dir), start(st), end(en), body(bdy) {
    if (st)
        addChild(st);
    if (en)
        addChild(en);
    if (bdy)
        addChild(bdy);
}

std::string ForNode::label() const {
    return "ForNode(" + loopVar + " " + direction + ")";
}

std::string ForNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:void";
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

RepeatNode::RepeatNode(ASTNode *bdy, ASTNode *cond)
    : body(bdy), condition(cond) {
    if (bdy)
        addChild(bdy);
    if (cond)
        addChild(cond);
}

std::string RepeatNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:void";
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

CaseBranchNode::CaseBranchNode(const std::vector<ASTNode *> &lbls, ASTNode *stmt)
    : labels(lbls), statement(stmt) {
    for (auto lbl : lbls) {
        if (lbl)    
            addChild(lbl);
    }
    if (stmt)
        addChild(stmt);
}

CaseNode::CaseNode(ASTNode *sel, const std::vector<CaseBranchNode *> &brs)
    : selector(sel), branches(brs) {
    if (sel)
        addChild(sel);
    for (auto br : brs) {
        if (br)
            addChild(br);
    }
}

std::string CaseNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:void";
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

EmptyNode::EmptyNode() {}

BinOpNode::BinOpNode(const std::string &oper, ASTNode *l, ASTNode *r)
    : op(oper), left(l), right(r) {
    if (l)
        addChild(l);
    if (r)
        addChild(r);
}

std::string BinOpNode::label() const {
    return "BinOp '" + op + "'";
}

std::string BinOpNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << (type.empty() ? "?" : type);
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

UnaryOpNode::UnaryOpNode(const std::string &oper, ASTNode *opnd)
    : op(oper), operand(opnd) {
    if (opnd)
        addChild(opnd);
}

std::string UnaryOpNode::label() const {
    return "UnaryOp '" + op + "'";
}

std::string UnaryOpNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << (type.empty() ? "?" : type);
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

VarNode::VarNode(const std::string &n) : name(n) {}

std::string VarNode::label() const {
    return "'" + name + "'";
}

std::string VarNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  tab_index:" << tabRef << ", type:" << (type.empty() ? "?" : type);
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

ArrayAccessNode::ArrayAccessNode(ASTNode *arr, ASTNode *idx)
    : array(arr), index(idx), lowBound(0), highBound(0), elementSize(1),
      totalSize(1), elementTypeName(""), elementTypeNode(nullptr) {
    if (arr)
        addChild(arr);
    if (idx)
        addChild(idx);
}

std::string ArrayAccessNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << (type.empty() ? "?" : type);
    if (level > 0)
        oss << ", lev:" << level;
    if (tabRef > 0)
        oss << ", tab_index:" << tabRef;
    oss << ", low:" << lowBound << ", elem_size:" << elementSize;
    return oss.str();
}

RecordAccessNode::RecordAccessNode(ASTNode *rec, const std::string &fld)
    : record(rec), field(fld), fieldOffset(0), fieldSize(1),
      fieldTypeName(""), fieldTypeNode(nullptr) {
    if (rec)
        addChild(rec);
}

std::string RecordAccessNode::label() const {
    return ". " + field;
}

std::string RecordAccessNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << (type.empty() ? "?" : type);
    if (level > 0)
        oss << ", lev:" << level;
    oss << ", field_offset:" << fieldOffset;
    if (fieldSize != 1)
        oss << ", size:" << fieldSize;
    return oss.str();
}

NumberNode::NumberNode(int val) : value(val) {
    type = "Integer";
}

std::string NumberNode::label() const {
    return std::to_string(value);
}

std::string NumberNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << type;
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

RealNode::RealNode(double val) : value(val) {
    type = "Real";
}

std::string RealNode::label() const {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string RealNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << type;
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

BooleanNode::BooleanNode(bool val) : value(val) {
    type = "Boolean";
}

std::string BooleanNode::label() const {
    return value ? "true" : "false";
}

std::string BooleanNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << type;
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

StringNode::StringNode(const std::string &val) : value(val) {
    type = "String";
}

std::string StringNode::label() const {
    return "'" + value + "'";
}

std::string StringNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << type;
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

CharNode::CharNode(char val) : value(val) {
    type = "Char";
}

std::string CharNode::label() const {
    return "'" + std::string(1, value) + "'";
}

std::string CharNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << type;
    if (level > 0)
        oss << ", lev:" << level;
    return oss.str();
}

FuncCallNode::FuncCallNode(const std::string &name, const std::vector<ASTNode *> &arguments)
    : funcName(name), args(arguments) {
    for (auto arg : args) {
        if (arg)
            addChild(arg);
    }
}

std::string FuncCallNode::label() const {
    return "FuncCall(" + funcName + ")";
}

std::string FuncCallNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:" << (type.empty() ? "?" : type);
    if (tabRef > 0)
        oss << ", tab_index:" << tabRef;
    return oss.str();
}

SubprogramDeclNode::SubprogramDeclNode(const std::string &k, const std::string &n,
                                       const std::vector<VarDeclNode *> &p,
                                       const std::string &retType, ASTNode *bdy)
    : kind(k), subName(n), returnType(retType), params(p), body(bdy) {
    for (auto param : params) {
        if (param)
            addChild(param);
    }
    if (bdy)
        addChild(bdy);
}

std::string SubprogramDeclNode::label() const {
    if (kind == "function") {
        return kind + " " + subName + " : " + returnType;
    }
    return kind + " " + subName;
}

std::string SubprogramDeclNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  tab_index:" << tabRef << ", lev:" << level;
    return oss.str();
}

ArrayTypeNode::ArrayTypeNode(ASTNode *idxType, ASTNode *elemType)
    : indexType(idxType), elementType(elemType), lowBound(0), highBound(0),
      elementSize(1), totalSize(1), elementTypeName("") {
    if (idxType)
        addChild(idxType);
    if (elemType)
        addChild(elemType);
}

std::string ArrayTypeNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:array";
    if (tabRef > 0)
        oss << ", atab_index:" << tabRef;
    oss << ", bounds:" << lowBound << ".." << highBound
        << ", elem_size:" << elementSize << ", size:" << totalSize;
    return oss.str();
}

RecordTypeNode::RecordTypeNode(const std::vector<VarDeclNode *> &flds)
    : fields(flds), totalSize(0) {
    for (auto f : fields) {
        if (f)
            addChild(f);
    }
}

std::string RecordTypeNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:record";
    if (tabRef > 0)
        oss << ", btab_index:" << tabRef;
    oss << ", size:" << totalSize;
    return oss.str();
}

RangeNode::RangeNode(ASTNode *l, ASTNode *h)
    : low(l), high(h), lowValue(0), highValue(0), hasBounds(false) {
    if (l)
        addChild(l);
    if (h)
        addChild(h);
}

std::string RangeNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:subrange" << (type.empty() ? "" : "(" + type + ")");
    if (hasBounds)
        oss << ", bounds:" << lowValue << ".." << highValue;
    return oss.str();
}

EnumeratedNode::EnumeratedNode(const std::vector<std::string> &vals)
    : values(vals) {}

std::string EnumeratedNode::label() const {
    std::ostringstream oss;
    oss << "Enumerated(";
    for (std::size_t i = 0; i < values.size(); i++) {
        if (i > 0)
            oss << ", ";
        oss << values[i];
    }
    oss << ")";
    return oss.str();
}

std::string EnumeratedNode::annotation() const {
    std::ostringstream oss;
    oss << "  ->  type:enumerated";
    if (!type.empty())
        oss << "(" << type << ")";
    return oss.str();
}
