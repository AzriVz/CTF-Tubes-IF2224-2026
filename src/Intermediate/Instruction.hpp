#ifndef __INSTRUCTION_HPP__
#define __INSTRUCTION_HPP__

#include <iostream>
#include <string>

struct Instruction {
    int line;
    std::string op;
    int level;
    std::string argument;

    Instruction();
    Instruction(int ln, const std::string &opcode, int lev, const std::string &arg);

    std::string toString() const;
};

std::ostream &operator<<(std::ostream &out, const Instruction &instruction);

#endif
