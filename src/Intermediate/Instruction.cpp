#include "Instruction.hpp"
#include <sstream>

Instruction::Instruction() : line(0), op(""), level(0), argument("0") {}

Instruction::Instruction(int ln, const std::string &opcode, int lev, const std::string &arg)
    : line(ln), op(opcode), level(lev), argument(arg) {}

std::string Instruction::toString() const {
    std::ostringstream oss;
    oss << line << " " << op << " " << level << " " << argument;
    return oss.str();
}

std::ostream &operator<<(std::ostream &out, const Instruction &instruction) {
    out << instruction.toString();
    return out;
}
