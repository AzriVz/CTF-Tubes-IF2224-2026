CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -I src

SRC_DIR = src
LEXER_DIR = src/Lexer
BIN_DIR = bin
INPUT_DIR = test/milestone-4/input

SRCS = $(sort $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp) $(wildcard $(LEXER_DIR)/*.cpp))
OBJS = $(patsubst %.cpp, $(BIN_DIR)/%.o, $(SRCS))

TARGET = $(BIN_DIR)/arion-compiler
TARGET_INPUT := $(if $(word 2,$(MAKECMDGOALS)),$(word 2,$(MAKECMDGOALS)),$(INPUT_DIR)/input-1.txt)

.PHONY: all build run parse lexer semantic ic clean

all: $(TARGET)

build: all

run: all
	./$(TARGET) $(TARGET_INPUT)

parse: all
	./$(TARGET) --parse $(TARGET_INPUT)

lexer: all
	./$(TARGET) --lexer $(TARGET_INPUT)

semantic: all
	./$(TARGET) --semantic $(TARGET_INPUT)

ic: all
	./$(TARGET) --ic $(TARGET_INPUT)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)

%:
	@:
