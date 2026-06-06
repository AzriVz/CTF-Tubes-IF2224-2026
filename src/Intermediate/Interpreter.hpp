#ifndef __INTERPRETER_HPP__
#define __INTERPRETER_HPP__

#include "Instruction.hpp"
#include <map>
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

    struct FrameInfo {
        int base;
        int limit;

        FrameInfo(int b = 0, int l = -1) : base(b), limit(l) {}
    };

    std::vector<StackValue> stack;
    std::vector<std::string> errors;
    std::map<int, FrameInfo> frames;
    int pc;
    int sp;
    int currentBase;
    bool halted;
    int maxSteps;
    int callDepth;
    int maxCallDepth;

    int base(int level);
    bool ensureIndex(int index);
    bool validateProgram(const std::vector<Instruction> &instructions);
    bool validateJumpTarget(int target, int instructionCount, const std::string &op, int line);
    bool validateDataAddress(int index, const std::string &action, int line);
    bool validateDataBlock(int start, int size, const std::string &action, int line);
    bool pushIntChecked(long long value, const std::string &context);
    bool push(const StackValue &value);
    bool pop(StackValue &value);
    bool popBinary(StackValue &left, StackValue &right);
    int parseIntArgument(const Instruction &instruction);
    void parseCallArgument(const Instruction &instruction, int &target, int &paramCount);
    StackValue parseLiteral(const std::string &argument);
    StackValue parseInputValue(const std::string &token);
    void executeOperation(int operation, std::ostream &out, int line);
    void addError(const std::string &message);

public:
    StackMachineInterpreter(int stackSize = 2048, int stepLimit = 1000000, int callDepthLimit = 1000);

    void reset();
    void execute(const std::vector<Instruction> &instructions, std::ostream &out);

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string> &getErrors() const { return errors; }
};

#endif
