CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -I src

SRC_DIR = src
LEXER_DIR = src/Lexer
BIN_DIR = bin
INPUT_DIR = test/milestone-1

SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(LEXER_DIR)/*.cpp)
OBJS = $(patsubst %.cpp, $(BIN_DIR)/%.o, $(SRCS))

TARGET = $(BIN_DIR)/arion-compiler

all: $(TARGET)

run: all
	./$(TARGET) $(INPUT_DIR)/input-1.txt

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)