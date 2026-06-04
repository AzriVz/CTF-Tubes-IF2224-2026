#include "Interpreter.hpp"
#include <cstdlib>
#include <iomanip>
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
    if (type == STRING)
        return stringValue.empty() ? 0 : 1;
    return intValue;
}

double StackMachineInterpreter::StackValue::asReal() const {
    if (type == REAL)
        return realValue;
    if (type == STRING)
        return stringValue.empty() ? 0.0 : 1.0;
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

StackMachineInterpreter::StackMachineInterpreter(int stackSize, int stepLimit)
    : stack(stackSize), pc(0), sp(-1), currentBase(0), halted(false), maxSteps(stepLimit) {}

void StackMachineInterpreter::reset() {
    for (size_t i = 0; i < stack.size(); i++) {
        stack[i] = StackValue();
    }
    errors.clear();
    pc = 0;
    sp = -1;
    currentBase = 0;
    halted = false;
}

void StackMachineInterpreter::execute(const vector<Instruction> &instructions, ostream &out) {
    reset();

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
            if (!ensureIndex(newSp))
                break;

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
        }
        else if (instruction.op == "LIT") {
            if (!push(parseLiteral(instruction.argument)))
                break;
        }
        else if (instruction.op == "LOD") {
            int resolvedBase = base(instruction.level);
            int address = parseIntArgument(instruction);
            int index = resolvedBase + address;
            if (!errors.empty() || !ensureIndex(index))
                break;
            if (!push(stack[index]))
                break;
        }
        else if (instruction.op == "STO") {
            int resolvedBase = base(instruction.level);
            int address = parseIntArgument(instruction);
            int index = resolvedBase + address;
            StackValue value;
            if (!errors.empty() || !ensureIndex(index) || !pop(value))
                break;
            stack[index] = value;
        }
        else if (instruction.op == "JMP") {
            int target = parseIntArgument(instruction);
            if (target < 0 || target > (int)instructions.size()) {
                addError("Invalid JMP target " + to_string(target) + " at line " + to_string(instruction.line));
                break;
            }
            pc = target;
        }
        else if (instruction.op == "JPC") {
            int target = parseIntArgument(instruction);
            StackValue condition;
            if (!pop(condition))
                break;
            if (condition.asInt() == 0) {
                if (target < 0 || target > (int)instructions.size()) {
                    addError("Invalid JPC target " + to_string(target) + " at line " + to_string(instruction.line));
                    break;
                }
                pc = target;
            }
        }
        else if (instruction.op == "CAL") {
            int target = parseIntArgument(instruction);
            if (target < 0 || target >= (int)instructions.size()) {
                addError("Invalid CAL target " + to_string(target) + " at line " + to_string(instruction.line));
                break;
            }
            if (!ensureIndex(sp + 3))
                break;
            int staticLink = base(instruction.level);
            if (!errors.empty())
                break;
            stack[sp + 1] = StackValue::fromInt(staticLink);
            stack[sp + 2] = StackValue::fromInt(currentBase);
            stack[sp + 3] = StackValue::fromInt(pc);
            currentBase = sp + 1;
            pc = target;
        }
        else if (instruction.op == "OPR") {
            executeOperation(parseIntArgument(instruction), out);
            if (!errors.empty())
                break;
        }
        else if (instruction.op == "RET") {
            if (currentBase == 0) {
                sp = -1;
                halted = true;
            }
            else {
                if (!ensureIndex(currentBase + 2))
                    break;
                int returnPc = stack[currentBase + 2].asInt();
                int dynamicLink = stack[currentBase + 1].asInt();
                sp = currentBase - 1;
                pc = returnPc;
                currentBase = dynamicLink;
            }
        }
        else {
            addError("Unknown opcode '" + instruction.op + "' at line " + to_string(instruction.line));
            break;
        }
    }

    if (!halted && errors.empty() && pc < 0) {
        addError("Program counter moved before the first instruction");
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

bool StackMachineInterpreter::ensureIndex(int index) {
    if (index < 0 || index >= (int)stack.size()) {
        addError("Stack access out of bounds at index " + to_string(index));
        return false;
    }
    return true;
}

bool StackMachineInterpreter::push(const StackValue &value) {
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
    return atoi(instruction.argument.c_str());
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

    return StackValue::fromInt(atoi(argument.c_str()));
}

void StackMachineInterpreter::executeOperation(int operation, ostream &out) {
    if (operation == 1) {
        if (sp < 0) {
            addError("Stack underflow on NEG");
            return;
        }
        if (stack[sp].type == StackValue::REAL)
            stack[sp] = StackValue::fromReal(-stack[sp].realValue);
        else
            stack[sp] = StackValue::fromInt(-stack[sp].asInt());
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
                push(StackValue::fromInt(a + b));
            }
            break;
        case 3:
            push(realResult ? StackValue::fromReal(ar - br) : StackValue::fromInt(a - b));
            break;
        case 4:
            push(realResult ? StackValue::fromReal(ar * br) : StackValue::fromInt(a * b));
            break;
        case 5:
            if ((realResult && br == 0.0) || (!realResult && b == 0)) {
                addError("Division by zero");
                return;
            }
            push(realResult ? StackValue::fromReal(ar / br) : StackValue::fromInt(a / b));
            break;
        case 6:
            if (b == 0) {
                addError("Modulo by zero");
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
