#include "SymbolTable.hpp"
#include <algorithm>

const std::vector<std::string> SymbolTable::reservedWords = {
    "and",
    "array",
    "begin",
    "case",
    "const",
    "div",
    "downto",
    "do",
    "else",
    "end",
    "for",
    "function",
    "if",
    "mod",
    "not",
    "of",
    "or",
    "procedure",
    "program",
    "record",
    "repeat",
    "integer",
    "real",
    "boolean",
    "char",
    "string",
    "then",
    "to",
    "type",
    "until",
    "var",
    "while"};

SymbolTable::SymbolTable()
    : typeInteger(0), typeReal(0), typeBoolean(0), typeChar(0), typeString(0) {}

void SymbolTable::init()
{
    tab.clear();
    btab.clear();
    atab.clear();
    display.clear();

    btab.push_back(BtabEntry(0, 0, 0, 0));
    display.push_back(0);

    initReservedWords();

    initPredefinedIdentifiers();
}

void SymbolTable::initReservedWords()
{
    for (std::size_t i = 0; i < reservedWords.size(); i++)
    {
        TabEntry entry;
        entry.id = reservedWords[i];
        entry.obj = "reserved";
        entry.type = 0;
        entry.ref = 0;
        entry.nrm = 1;
        entry.lev = 0;
        entry.adr = 0;

        entry.link = btab[0].last;

        int idx = (int)tab.size();
        tab.push_back(entry);
        btab[0].last = idx;
    }
}

void SymbolTable::initPredefinedIdentifiers()
{
    typeInteger = (int)tab.size();
    insertTab("Integer", "type", typeInteger, 0, 1, 0, 0);

    typeReal = (int)tab.size();
    insertTab("Real", "type", typeReal, 0, 1, 0, 0);

    typeBoolean = (int)tab.size();
    insertTab("Boolean", "type", typeBoolean, 0, 1, 0, 0);

    typeChar = (int)tab.size();
    insertTab("Char", "type", typeChar, 0, 1, 0, 0);

    typeString = (int)tab.size();
    insertTab("String", "type", typeString, 0, 1, 0, 0);

    insertTab("True", "constant", typeBoolean, 0, 1, 0, 1);

    insertTab("False", "constant", typeBoolean, 0, 1, 0, 0);

    insertTab("Writeln", "procedure", 0, 0, 1, 0, 0);

    insertTab("Readln", "procedure", 0, 0, 1, 0, 0);
}

int SymbolTable::insertTab(const std::string &id, const std::string &obj, int type,
                           int ref, int nrm, int lev, int adr)
{
    if (lev < 0)
    {
        lev = currentLevel();
    }

    int curBlock = display[lev];
    int prevLast = btab[curBlock].last;

    TabEntry entry(id, obj, type, ref, nrm, lev, adr, prevLast);
    int idx = (int)tab.size();
    tab.push_back(entry);

    btab[curBlock].last = idx;

    return idx;
}

int SymbolTable::insertBtab(int last, int lpar, int psze, int vsze)
{
    int idx = (int)btab.size();
    btab.push_back(BtabEntry(last, lpar, psze, vsze));
    return idx;
}

int SymbolTable::insertAtab(int xtyp, int etyp, int eref, int low, int high,
                            int elsz, int size)
{
    int idx = (int)atab.size();
    atab.push_back(AtabEntry(idx, xtyp, etyp, eref, low, high, elsz, size));
    return idx;
}

int SymbolTable::lookup(const std::string &id) const
{

    int curLev = currentLevel();
    while (curLev >= 0)
    {
        int result = lookupInScope(id, curLev);
        if (result >= 0)
        {
            return result;
        }
        curLev--;
    }

    // Fallback: try with capitalized first letter
    // (handles cases like "writeln" matching "Writeln")
    if (!id.empty())
    {
        std::string cap = id;
        cap[0] = std::toupper(static_cast<unsigned char>(cap[0]));
        if (cap != id)
        {
            return lookup(cap);
        }
    }

    return -1;
}

int SymbolTable::lookupInScope(const std::string &id, int level) const
{
    if (level < 0 || level >= (int)display.size())
        return -1;

    int blockIdx = display[level];
    int curIdx = btab[blockIdx].last;

    while (curIdx > 0)
    {
        if (tab[curIdx].id == id)
        {
            return curIdx;
        }
        curIdx = tab[curIdx].link;
    }

    if (curIdx == 0 && tab[0].id == id)
    {
        return 0;
    }

    return -1;
}

int SymbolTable::pushScope()
{
    int newBlockIdx = insertBtab(0, 0, 0, 0);
    display.push_back(newBlockIdx);
    return newBlockIdx;
}

void SymbolTable::popScope()
{
    if (display.size() > 1)
    {
        display.pop_back();
    }
}

int SymbolTable::currentBlock() const
{
    if (display.empty())
        return -1;
    return display.back();
}

const TabEntry &SymbolTable::getTabEntry(int idx) const
{
    return tab[idx];
}

const BtabEntry &SymbolTable::getBtabEntry(int idx) const
{
    return btab[idx];
}

const AtabEntry &SymbolTable::getAtabEntry(int idx) const
{
    return atab[idx];
}

int SymbolTable::getTypeCode(const std::string &typeName) const
{
    // Direct lookup
    int idx = lookup(typeName);
    if (idx >= 0 && tab[idx].obj == "type")
    {
        return tab[idx].type; // For type entries, return the base type code
    }

    // If found as reserved word (e.g. lowercase "integer"), try capitalized version
    if (idx >= 0 && tab[idx].obj == "reserved" && !typeName.empty())
    {
        std::string cap = typeName;
        cap[0] = std::toupper(static_cast<unsigned char>(cap[0]));
        int idx2 = lookup(cap);
        if (idx2 >= 0 && tab[idx2].obj == "type")
        {
            return tab[idx2].type;
        }
    }

    // Try capitalized version directly
    if (!typeName.empty())
    {
        std::string cap = typeName;
        cap[0] = std::toupper(static_cast<unsigned char>(cap[0]));
        int idx2 = lookup(cap);
        if (idx2 >= 0 && tab[idx2].obj == "type")
        {
            return tab[idx2].type;
        }
    }

    return 0;
}

void SymbolTable::setBlockLast(int tabIdx)
{
    int block = currentBlock();
    if (block >= 0)
    {
        btab[block].last = tabIdx;
    }
}

void SymbolTable::setBlockLpar(int tabIdx)
{
    int block = currentBlock();
    if (block >= 0)
    {
        btab[block].lpar = tabIdx;
    }
}

void SymbolTable::addVarSize(int size)
{
    int block = currentBlock();
    if (block >= 0)
    {
        btab[block].vsze += size;
    }
}

void SymbolTable::addParamSize(int size)
{
    int block = currentBlock();
    if (block >= 0)
    {
        btab[block].psze += size;
    }
}

int SymbolTable::getCurrentVarSize() const
{
    int block = currentBlock();
    if (block >= 0)
    {
        return btab[block].vsze;
    }
    return 0;
}

int SymbolTable::getCurrentParamSize() const
{
    int block = currentBlock();
    if (block >= 0)
    {
        return btab[block].psze;
    }
    return 0;
}

std::string SymbolTable::objToString(const std::string &obj)
{
    if (obj == "variable")
        return "variable";
    if (obj == "constant")
        return "constant";
    if (obj == "type")
        return "type";
    if (obj == "procedure")
        return "procedure";
    if (obj == "function")
        return "function";
    if (obj == "program")
        return "program";
    if (obj == "parameter")
        return "parameter";
    if (obj == "reserved")
        return "reserved";
    return obj;
}

void SymbolTable::printTab(std::ostream &out) const
{
    out << "=== tab (Identifier Table) ===" << std::endl;
    out << std::left
        << std::setw(6) << "idx"
        << std::setw(16) << "id"
        << std::setw(14) << "obj"
        << std::setw(6) << "type"
        << std::setw(6) << "ref"
        << std::setw(6) << "nrm"
        << std::setw(6) << "lev"
        << std::setw(6) << "adr"
        << std::setw(6) << "link"
        << std::endl;
    out << std::string(70, '-') << std::endl;

    for (std::size_t i = 0; i < tab.size(); i++)
    {
        const auto &e = tab[i];
        out << std::left
            << std::setw(6) << i
            << std::setw(16) << e.id
            << std::setw(14) << e.obj
            << std::setw(6) << e.type
            << std::setw(6) << e.ref
            << std::setw(6) << e.nrm
            << std::setw(6) << e.lev
            << std::setw(6) << e.adr
            << std::setw(6) << e.link
            << std::endl;
    }
    out << std::endl;
}

void SymbolTable::printBtab(std::ostream &out) const
{
    out << "=== btab (Block Table) ===" << std::endl;
    out << std::left
        << std::setw(6) << "idx"
        << std::setw(8) << "last"
        << std::setw(8) << "lpar"
        << std::setw(8) << "psze"
        << std::setw(8) << "vsze"
        << std::endl;
    out << std::string(40, '-') << std::endl;

    for (std::size_t i = 0; i < btab.size(); i++)
    {
        const auto &e = btab[i];
        out << std::left
            << std::setw(6) << i
            << std::setw(8) << e.last
            << std::setw(8) << e.lpar
            << std::setw(8) << e.psze
            << std::setw(8) << e.vsze
            << std::endl;
    }
    out << std::endl;
}

void SymbolTable::printAtab(std::ostream &out) const
{
    out << "=== atab (Array Table) ===" << std::endl;
    if (atab.empty())
    {
        out << "(empty)" << std::endl;
        out << std::endl;
        return;
    }

    out << std::left
        << std::setw(8) << "arrays"
        << std::setw(8) << "xtyp"
        << std::setw(8) << "etyp"
        << std::setw(8) << "eref"
        << std::setw(8) << "low"
        << std::setw(8) << "high"
        << std::setw(8) << "elsz"
        << std::setw(8) << "size"
        << std::endl;
    out << std::string(66, '-') << std::endl;

    for (std::size_t i = 0; i < atab.size(); i++)
    {
        const auto &e = atab[i];
        out << std::left
            << std::setw(8) << e.arrays
            << std::setw(8) << e.xtyp
            << std::setw(8) << e.etyp
            << std::setw(8) << e.eref
            << std::setw(8) << e.low
            << std::setw(8) << e.high
            << std::setw(8) << e.elsz
            << std::setw(8) << e.size
            << std::endl;
    }
    out << std::endl;
}

void SymbolTable::printAll(std::ostream &out) const
{
    printTab(out);
    printBtab(out);
    printAtab(out);
}
