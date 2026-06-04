#ifndef __INTERPRETER_HPP__
#define __INTERPRETER_HPP__

#include "Instruction.hpp"
#include <ostream>
#include <string>
#include <vector>

class StackMachineInterpreter {
private:
    struct StackValue {
        enum ValueType {
            INTEGER,
            REAL,
            STRING
        };

        ValueType type;
        int intValue;
        double realValue;
        std::string stringValue;

        StackValue();
        static StackValue fromInt(int value);
        static StackValue fromReal(double value);
        static StackValue fromString(const std::string &value);

        int asInt() const;
        double asReal() const;
        std::string toOutputString() const;
    };

    std::vector<StackValue> stack;
    std::vector<std::string> errors;
    int pc;
    int sp;
    int currentBase;
    bool halted;
    int maxSteps;

    int base(int level);
    bool ensureIndex(int index);
    bool push(const StackValue &value);
    bool pop(StackValue &value);
    bool popBinary(StackValue &left, StackValue &right);
    int parseIntArgument(const Instruction &instruction);
    StackValue parseLiteral(const std::string &argument);
    void executeOperation(int operation, std::ostream &out);
    void addError(const std::string &message);

public:
    StackMachineInterpreter(int stackSize = 2048, int stepLimit = 1000000);

    void reset();
    void execute(const std::vector<Instruction> &instructions, std::ostream &out);

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string> &getErrors() const { return errors; }
};

#endif
