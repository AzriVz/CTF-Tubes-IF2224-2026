#ifndef __SYMBOLTABLE_HPP__
#define __SYMBOLTABLE_HPP__

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

struct TabEntry
{
    std::string id;
    std::string obj;

    int type;
    int ref;

    int nrm;
    int lev;
    int adr;

    int link;

    TabEntry() : type(0), ref(0), nrm(1), lev(0), adr(0), link(0) {}
    TabEntry(const std::string &i, const std::string &o, int t, int r,
             int n, int l, int a, int lk)
        : id(i), obj(o), type(t), ref(r), nrm(n), lev(l), adr(a), link(lk) {}
};

struct BtabEntry
{
    int last;
    int lpar;
    int psze;
    int vsze;

    BtabEntry() : last(0), lpar(0), psze(0), vsze(0) {}
    BtabEntry(int lst, int lp, int ps, int vs)
        : last(lst), lpar(lp), psze(ps), vsze(vs) {}
};

struct AtabEntry
{
    int arrays;
    int xtyp;
    int etyp;
    int eref;
    int low;
    int high;
    int elsz;
    int size;

    AtabEntry() : arrays(0), xtyp(0), etyp(0), eref(0),
                  low(0), high(0), elsz(0), size(0) {}
    AtabEntry(int a, int xt, int et, int er, int lo, int hi, int es, int sz)
        : arrays(a), xtyp(xt), etyp(et), eref(er),
          low(lo), high(hi), elsz(es), size(sz) {}
};

class SymbolTable
{
private:
    std::vector<TabEntry> tab;
    std::vector<BtabEntry> btab;
    std::vector<AtabEntry> atab;
    std::vector<int> display;

    int typeInteger;
    int typeReal;
    int typeBoolean;
    int typeChar;
    int typeString;

    static const std::vector<std::string> reservedWords;

    void initReservedWords();
    void initPredefinedIdentifiers();

public:
    SymbolTable();

    void init();

    int insertTab(const std::string &id, const std::string &obj, int type,
                  int ref = 0, int nrm = 1, int lev = -1, int adr = 0);

    int insertBtab(int last = 0, int lpar = 0, int psze = 0, int vsze = 0);

    int insertAtab(int xtyp, int etyp, int eref, int low, int high,
                   int elsz, int size);

    int lookup(const std::string &id) const;

    int lookupInScope(const std::string &id, int level) const;

    int pushScope();

    void popScope();

    int currentLevel() const { return display.size() - 1; }

    int currentBlock() const;

    const TabEntry &getTabEntry(int idx) const;
    const BtabEntry &getBtabEntry(int idx) const;
    const AtabEntry &getAtabEntry(int idx) const;

    int getTabSize() const { return (int)tab.size(); }
    int getBtabSize() const { return (int)btab.size(); }
    int getAtabSize() const { return (int)atab.size(); }

    int getTypeCode(const std::string &typeName) const;
    int getTypeInteger() const { return typeInteger; }
    int getTypeReal() const { return typeReal; }
    int getTypeBoolean() const { return typeBoolean; }
    int getTypeChar() const { return typeChar; }
    int getTypeString() const { return typeString; }

    void setBlockLast(int tabIdx);

    void setBlockLpar(int tabIdx);

    void addVarSize(int size = 1);

    void addParamSize(int size = 1);

    int getCurrentVarSize() const;

    int getCurrentParamSize() const;

    static std::string objToString(const std::string &obj);

    void printTab(std::ostream &out) const;

    void printBtab(std::ostream &out) const;

    void printAtab(std::ostream &out) const;

    void printAll(std::ostream &out) const;
};

#endif
