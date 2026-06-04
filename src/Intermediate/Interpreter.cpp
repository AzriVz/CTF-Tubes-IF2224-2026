#include "Interpreter.hpp"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

using namespace std;

StackMachineInterpreter::StackValue::StackValue()
    : type(INTEGER), intValue(0), realValue(0.0), stringValue("") {}

StackMachineInterpreter::StackValue StackMachineInterpreter::StackValue::fromInt(int value) {
    StackValue result;
    result.type = INTEGER;
    result.intValue = value;
    result.realValue = value;
    return result;
}

StackMachineInterpreter::StackValue StackMachineInterpreter::StackValue::fromReal(double value) {
    StackValue result;
    result.type = REAL;
    result.intValue = (int)value;
    result.realValue = value;
    return result;
}

StackMachineInterpreter::StackValue StackMachineInterpreter::StackValue::fromString(const string &value) {
    StackValue result;
    result.type = STRING;
    result.stringValue = value;
    return result;
}

int StackMachineInterpreter::StackValue::asInt() const {
    if (type == REAL)
        return (int)realValue;
    if (type == STRING) {
        if (stringValue.size() == 1)
            return (unsigned char)stringValue[0];
        return stringValue.empty() ? 0 : 1;
    }
    return intValue;
}

double StackMachineInterpreter::StackValue::asReal() const {
    if (type == REAL)
        return realValue;
    if (type == STRING) {
        if (stringValue.size() == 1)
            return (unsigned char)stringValue[0];
        return stringValue.empty() ? 0.0 : 1.0;
    }
    return intValue;
}

string StackMachineInterpreter::StackValue::toOutputString() const {
    if (type == STRING)
        return stringValue;
    if (type == REAL) {
        ostringstream oss;
        oss << setprecision(15) << realValue;
        return oss.str();
    }
    return to_string(intValue);
}

StackMachineInterpreter::StackMachineInterpreter(int stackSize, int stepLimit, int callDepthLimit)
    : stack(stackSize), pc(0), sp(-1), currentBase(0), halted(false),
      maxSteps(stepLimit), callDepth(0), maxCallDepth(callDepthLimit) {}

void StackMachineInterpreter::reset() {
    for (size_t i = 0; i < stack.size(); i++) {
        stack[i] = StackValue();
    }
    errors.clear();
    frames.clear();
    pc = 0;
    sp = -1;
    currentBase = 0;
    halted = false;
    callDepth = 0;
}

void StackMachineInterpreter::execute(const vector<Instruction> &instructions, ostream &out) {
    reset();
    if (!validateProgram(instructions))
        return;

    int steps = 0;
    while (!halted && pc >= 0 && pc < (int)instructions.size()) {
        if (steps++ > maxSteps) {
            addError("Execution stopped because the step limit was exceeded");
            break;
        }

        Instruction instruction = instructions[pc];
        pc++;

        if (instruction.op == "INT") {
            int frameSize = parseIntArgument(instruction);
            if (frameSize < 3) {
                addError("INT frame size must be at least 3 at line " + to_string(instruction.line));
                break;
            }

            int newSp = currentBase + frameSize - 1;
            if (newSp < 0 || newSp >= (int)stack.size()) {
                addError("Stack overflow: INT frame allocation exceeds stack size at line " +
                         to_string(instruction.line));
                break;
            }

            if (currentBase == 0 && sp == -1) {
                for (int i = 0; i <= newSp; i++) {
                    stack[i] = StackValue::fromInt(0);
                }
            }
            else {
                for (int i = sp + 1; i <= newSp; i++) {
                    if (i >= currentBase && i <= currentBase + 2)
                        continue;
                    stack[i] = StackValue::fromInt(0);
                }
            }

            sp = newSp;
            frames[currentBase] = FrameInfo(currentBase, newSp);
        }
        else if (instruction.op == "LIT") {
            if (!push(parseLiteral(instruction.argument)))
                break;
        }
        else if (instruction.op == "LOD") {
            int resolvedBase = base(instruction.level);
            int address = parseIntArgument(instruction);
            int index = resolvedBase + address;
            if (!errors.empty() || !validateDataAddress(index, "read", instruction.line))
                break;
            if (!push(stack[index]))
                break;
        }
        else if (instruction.op == "STO") {
            int resolvedBase = base(instruction.level);
            int address = parseIntArgument(instruction);
            int index = resolvedBase + address;
            StackValue value;
            if (!errors.empty() || !validateDataAddress(index, "write", instruction.line) || !pop(value))
                break;
            stack[index] = value;
        }
        else if (instruction.op == "LDA") {
            int resolvedBase = base(instruction.level);
            int address = parseIntArgument(instruction);
            int index = resolvedBase + address;
            if (!errors.empty() || !validateDataAddress(index, "address", instruction.line))
                break;
            if (!push(StackValue::fromInt(index)))
                break;
        }
        else if (instruction.op == "LDI") {
            StackValue address;
            if (!pop(address))
                break;
            int index = address.asInt();
            if (!validateDataAddress(index, "indirect read", instruction.line))
                break;
            if (!push(stack[index]))
                break;
        }
        else if (instruction.op == "STI") {
            StackValue address;
            StackValue value;
            if (!pop(address) || !pop(value))
                break;
            int index = address.asInt();
            if (!validateDataAddress(index, "indirect write", instruction.line))
                break;
            stack[index] = value;
        }
        else if (instruction.op == "CPY") {
            int size = parseIntArgument(instruction);
            StackValue destination;
            StackValue source;
            if (size <= 0) {
                addError("Invalid CPY size " + to_string(size) + " at line " + to_string(instruction.line));
                break;
            }
            if (!pop(destination) || !pop(source))
                break;
            int destinationIndex = destination.asInt();
            int sourceIndex = source.asInt();
            if (!validateDataBlock(sourceIndex, size, "copy source", instruction.line) ||
                !validateDataBlock(destinationIndex, size, "copy destination", instruction.line))
                break;
            vector<StackValue> copied;
            for (int i = 0; i < size; i++) {
                copied.push_back(stack[sourceIndex + i]);
            }
            for (int i = 0; i < size; i++) {
                stack[destinationIndex + i] = copied[i];
            }
        }
        else if (instruction.op == "CHK") {
            if (sp < 0) {
                addError("Stack underflow on CHK at line " + to_string(instruction.line));
                break;
            }
            int low = instruction.level;
            int high = parseIntArgument(instruction);
            int value = stack[sp].asInt();
            if (value < low || value > high) {
                addError("IndexOutOfBoundsException: index " + to_string(value) +
                         " is outside bounds " + to_string(low) + ".." +
                         to_string(high) + " at line " + to_string(instruction.line));
                break;
            }
        }
        else if (instruction.op == "REA") {
            string token;
            if (!(cin >> token)) {
                addError("Input expected for READLN at line " + to_string(instruction.line));
                break;
            }
            if (!push(parseInputValue(token)))
                break;
        }
        else if (instruction.op == "JMP") {
            int target = parseIntArgument(instruction);
            if (!validateJumpTarget(target, (int)instructions.size(), "JMP", instruction.line))
                break;
            pc = target;
        }
        else if (instruction.op == "JPC") {
            int target = parseIntArgument(instruction);
            StackValue condition;
            if (!pop(condition))
                break;
            if (condition.asInt() == 0) {
                if (!validateJumpTarget(target, (int)instructions.size(), "JPC", instruction.line))
                    break;
                pc = target;
            }
        }
        else if (instruction.op == "CAL") {
            int target = 0;
            int paramCount = 0;
            parseCallArgument(instruction, target, paramCount);
            if (!validateJumpTarget(target, (int)instructions.size(), "CAL", instruction.line))
                break;
            if (paramCount < 0 || paramCount > sp + 1) {
                addError("Invalid parameter count " + to_string(paramCount) + " at line " + to_string(instruction.line));
                break;
            }
            if (callDepth + 1 > maxCallDepth) {
                addError("Stack overflow: maximum call depth " + to_string(maxCallDepth) +
                         " exceeded at line " + to_string(instruction.line));
                break;
            }

            int newBase = sp - paramCount + 1;
            if (paramCount == 0)
                newBase = sp + 1;

            if (newBase < 0 || newBase + 2 + paramCount >= (int)stack.size()) {
                addError("Stack overflow: CAL frame header exceeds stack size at line " +
                         to_string(instruction.line));
                break;
            }
            int staticLink = base(instruction.level);
            if (!errors.empty())
                break;

            for (int i = paramCount - 1; i >= 0; i--) {
                stack[newBase + 3 + i] = stack[newBase + i];
            }

            stack[newBase] = StackValue::fromInt(staticLink);
            stack[newBase + 1] = StackValue::fromInt(currentBase);
            stack[newBase + 2] = StackValue::fromInt(pc);
            currentBase = newBase;
            sp = newBase + 2 + paramCount;
            callDepth++;
            pc = target;
        }
        else if (instruction.op == "OPR") {
            executeOperation(parseIntArgument(instruction), out, instruction.line);
            if (!errors.empty())
                break;
        }
        else if (instruction.op == "RET") {
            auto frame = frames.find(currentBase);
            if (frame == frames.end()) {
                addError("Stack corruption: missing frame metadata at base " + to_string(currentBase));
                break;
            }
            if (sp != frame->second.limit) {
                addError("Stack corruption: unbalanced stack at RET line " + to_string(instruction.line) +
                         " (sp=" + to_string(sp) + ", frame_limit=" +
                         to_string(frame->second.limit) + ")");
                break;
            }

            if (currentBase == 0) {
                sp = -1;
                halted = true;
            }
            else {
                if (!ensureIndex(currentBase + 2))
                    break;
                int returnPc = stack[currentBase + 2].asInt();
                int dynamicLink = stack[currentBase + 1].asInt();
                if (!validateJumpTarget(returnPc, (int)instructions.size(), "RET", instruction.line))
                    break;
                if (dynamicLink != 0 && frames.find(dynamicLink) == frames.end()) {
                    addError("Stack corruption: invalid dynamic link " + to_string(dynamicLink) +
                             " at line " + to_string(instruction.line));
                    break;
                }
                frames.erase(currentBase);
                sp = currentBase - 1;
                pc = returnPc;
                currentBase = dynamicLink;
                if (callDepth > 0)
                    callDepth--;
            }
        }
        else {
            addError("Unknown opcode '" + instruction.op + "' at line " + to_string(instruction.line));
            break;
        }
    }

    if (!halted && errors.empty()) {
        if (pc < 0) {
            addError("Program counter moved before the first instruction");
        }
        else if (pc >= (int)instructions.size()) {
            addError("Program counter moved past the last instruction without RET");
        }
    }
}

int StackMachineInterpreter::base(int level) {
    int result = currentBase;
    while (level > 0) {
        if (!ensureIndex(result))
            return 0;
        result = stack[result].asInt();
        level--;
    }
    return result;
}

bool StackMachineInterpreter::validateProgram(const vector<Instruction> &instructions) {
    for (size_t i = 0; i < instructions.size(); i++) {
        const Instruction &instruction = instructions[i];
        if (instruction.line != (int)i) {
            addError("Invalid instruction numbering: line field " + to_string(instruction.line) +
                     " appears at index " + to_string(i));
            return false;
        }

        const string &op = instruction.op;
        if (op == "JMP" || op == "JPC" || op == "CAL") {
            int target = 0;
            int paramCount = 0;
            if (op == "CAL")
                parseCallArgument(instruction, target, paramCount);
            else
                target = parseIntArgument(instruction);
            if (!errors.empty())
                return false;
            if (!validateJumpTarget(target, (int)instructions.size(), op, instruction.line))
                return false;
            if (op == "CAL" && paramCount < 0) {
                addError("Invalid CAL parameter slot count " + to_string(paramCount) +
                         " at line " + to_string(instruction.line));
                return false;
            }
        }
        else if (op != "INT" && op != "LIT" && op != "LOD" && op != "STO" &&
                 op != "LDA" && op != "LDI" && op != "STI" && op != "CPY" &&
                 op != "CHK" && op != "REA" && op != "OPR" && op != "RET") {
            addError("Unknown opcode '" + op + "' at line " + to_string(instruction.line));
            return false;
        }
    }
    return true;
}

bool StackMachineInterpreter::validateJumpTarget(int target, int instructionCount, const string &op, int line) {
    if (target < 0 || target >= instructionCount) {
        addError("Invalid " + op + " target " + to_string(target) +
                 " at line " + to_string(line));
        return false;
    }
    return true;
}

bool StackMachineInterpreter::validateDataAddress(int index, const string &action, int line) {
    if (!ensureIndex(index))
        return false;

    for (const auto &entry : frames) {
        int frameBase = entry.second.base;
        int frameLimit = entry.second.limit;
        if (index >= frameBase && index <= frameLimit) {
            if (index < frameBase + 3) {
                addError("Stack smashing detected: attempted " + action +
                         " of frame control slot " + to_string(index) +
                         " at line " + to_string(line));
                return false;
            }
            return true;
        }
    }

    addError("Stack corruption detected: attempted " + action +
             " of address " + to_string(index) +
             " outside active frame data at line " + to_string(line));
    return false;
}

bool StackMachineInterpreter::validateDataBlock(int start, int size, const string &action, int line) {
    if (size <= 0) {
        addError("Invalid block size " + to_string(size) + " for " + action +
                 " at line " + to_string(line));
        return false;
    }

    for (int i = 0; i < size; i++) {
        if (!validateDataAddress(start + i, action, line))
            return false;
    }
    return true;
}

bool StackMachineInterpreter::pushIntChecked(long long value, const string &context) {
    if (value > numeric_limits<int>::max()) {
        addError("OverflowError: integer overflow during " + context);
        return false;
    }
    if (value < numeric_limits<int>::min()) {
        addError("UnderflowError: integer underflow during " + context);
        return false;
    }
    return push(StackValue::fromInt((int)value));
}

bool StackMachineInterpreter::ensureIndex(int index) {
    if (index < 0 || index >= (int)stack.size()) {
        addError("Stack access out of bounds at index " + to_string(index));
        return false;
    }
    return true;
}

bool StackMachineInterpreter::push(const StackValue &value) {
    if (sp + 1 >= (int)stack.size()) {
        addError("Stack overflow: push exceeds stack size");
        return false;
    }
    if (!ensureIndex(sp + 1))
        return false;
    sp++;
    stack[sp] = value;
    return true;
}

bool StackMachineInterpreter::pop(StackValue &value) {
    if (sp < 0) {
        addError("Stack underflow");
        return false;
    }
    value = stack[sp];
    sp--;
    return true;
}

bool StackMachineInterpreter::popBinary(StackValue &left, StackValue &right) {
    if (!pop(right))
        return false;
    if (!pop(left))
        return false;
    return true;
}

int StackMachineInterpreter::parseIntArgument(const Instruction &instruction) {
    char *end = nullptr;
    errno = 0;
    long value = strtol(instruction.argument.c_str(), &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        addError("Invalid integer operand '" + instruction.argument +
                 "' at line " + to_string(instruction.line));
        return 0;
    }
    if (value > numeric_limits<int>::max()) {
        addError("OverflowError: integer operand too large at line " + to_string(instruction.line));
        return 0;
    }
    if (value < numeric_limits<int>::min()) {
        addError("UnderflowError: integer operand too small at line " + to_string(instruction.line));
        return 0;
    }
    return (int)value;
}

void StackMachineInterpreter::parseCallArgument(const Instruction &instruction, int &target, int &paramCount) {
    size_t separator = instruction.argument.find(':');
    if (separator == string::npos) {
        target = parseIntArgument(instruction);
        paramCount = 0;
        return;
    }

    Instruction targetInstruction(instruction.line, instruction.op, instruction.level,
                                  instruction.argument.substr(0, separator));
    Instruction paramInstruction(instruction.line, instruction.op, instruction.level,
                                 instruction.argument.substr(separator + 1));
    target = parseIntArgument(targetInstruction);
    paramCount = parseIntArgument(paramInstruction);
}

StackMachineInterpreter::StackValue StackMachineInterpreter::parseLiteral(const string &argument) {
    if (argument.size() >= 2 && argument[0] == '\'' && argument[argument.size() - 1] == '\'') {
        string result;
        for (size_t i = 1; i + 1 < argument.size(); i++) {
            if (argument[i] == '\\' && i + 2 < argument.size()) {
                i++;
            }
            result.push_back(argument[i]);
        }
        return StackValue::fromString(result);
    }

    if (argument.find('.') != string::npos) {
        return StackValue::fromReal(atof(argument.c_str()));
    }

    char *end = nullptr;
    errno = 0;
    long value = strtol(argument.c_str(), &end, 10);
    if (errno != 0 || !end || *end != '\0') {
        addError("Invalid integer literal '" + argument + "'");
        return StackValue::fromInt(0);
    }
    if (value > numeric_limits<int>::max()) {
        addError("OverflowError: integer literal too large");
        return StackValue::fromInt(0);
    }
    if (value < numeric_limits<int>::min()) {
        addError("UnderflowError: integer literal too small");
        return StackValue::fromInt(0);
    }
    return StackValue::fromInt((int)value);
}

StackMachineInterpreter::StackValue StackMachineInterpreter::parseInputValue(const string &token) {
    if (token.empty())
        return StackValue::fromString("");

    char *end = nullptr;
    errno = 0;
    long intValue = strtol(token.c_str(), &end, 10);
    if (errno == 0 && end && *end == '\0') {
        if (intValue > numeric_limits<int>::max()) {
            addError("OverflowError: input integer too large");
            return StackValue::fromInt(0);
        }
        if (intValue < numeric_limits<int>::min()) {
            addError("UnderflowError: input integer too small");
            return StackValue::fromInt(0);
        }
        return StackValue::fromInt((int)intValue);
    }

    errno = 0;
    end = nullptr;
    double realValue = strtod(token.c_str(), &end);
    if (errno == 0 && end && *end == '\0')
        return StackValue::fromReal(realValue);

    return StackValue::fromString(token);
}

void StackMachineInterpreter::executeOperation(int operation, ostream &out, int line) {
    if (operation == 1) {
        if (sp < 0) {
            addError("Stack underflow on NEG");
            return;
        }
        if (stack[sp].type == StackValue::REAL) {
            stack[sp] = StackValue::fromReal(-stack[sp].realValue);
        }
        else {
            int value = stack[sp].asInt();
            if (value == numeric_limits<int>::min()) {
                addError("OverflowError: integer overflow during NEG at line " + to_string(line));
                return;
            }
            stack[sp] = StackValue::fromInt(-value);
        }
        return;
    }

    if (operation >= 2 && operation <= 12) {
        StackValue left;
        StackValue right;
        if (!popBinary(left, right))
            return;

        bool realResult = left.type == StackValue::REAL || right.type == StackValue::REAL;
        int a = left.asInt();
        int b = right.asInt();
        double ar = left.asReal();
        double br = right.asReal();

        switch (operation) {
        case 2:
            if (left.type == StackValue::STRING || right.type == StackValue::STRING) {
                push(StackValue::fromString(left.toOutputString() + right.toOutputString()));
            }
            else if (realResult) {
                push(StackValue::fromReal(ar + br));
            }
            else {
                pushIntChecked((long long)a + b, "ADD at line " + to_string(line));
            }
            break;
        case 3:
            if (realResult)
                push(StackValue::fromReal(ar - br));
            else
                pushIntChecked((long long)a - b, "SUB at line " + to_string(line));
            break;
        case 4:
            if (realResult)
                push(StackValue::fromReal(ar * br));
            else
                pushIntChecked((long long)a * b, "MUL at line " + to_string(line));
            break;
        case 5:
            if ((realResult && br == 0.0) || (!realResult && b == 0)) {
                addError("Division by zero");
                return;
            }
            if (realResult) {
                push(StackValue::fromReal(ar / br));
            }
            else {
                if (a == numeric_limits<int>::min() && b == -1) {
                    addError("OverflowError: integer overflow during DIV at line " + to_string(line));
                    return;
                }
                push(StackValue::fromInt(a / b));
            }
            break;
        case 6:
            if (b == 0) {
                addError("Modulo by zero");
                return;
            }
            if (a == numeric_limits<int>::min() && b == -1) {
                addError("OverflowError: integer overflow during MOD at line " + to_string(line));
                return;
            }
            push(StackValue::fromInt(a % b));
            break;
        case 7:
            push(StackValue::fromInt(left.toOutputString() == right.toOutputString() ? 1 : 0));
            break;
        case 8:
            push(StackValue::fromInt(left.toOutputString() != right.toOutputString() ? 1 : 0));
            break;
        case 9:
            push(StackValue::fromInt(ar < br ? 1 : 0));
            break;
        case 10:
            push(StackValue::fromInt(ar >= br ? 1 : 0));
            break;
        case 11:
            push(StackValue::fromInt(ar > br ? 1 : 0));
            break;
        case 12:
            push(StackValue::fromInt(ar <= br ? 1 : 0));
            break;
        default:
            break;
        }
        return;
    }

    if (operation == 13 || operation == 14) {
        StackValue value;
        if (!pop(value))
            return;
        out << value.toOutputString();
        if (operation == 14)
            out << endl;
        return;
    }

    addError("Unknown OPR operation " + to_string(operation));
}

void StackMachineInterpreter::addError(const string &message) {
    errors.push_back("Runtime error: " + message);
    halted = true;
}
