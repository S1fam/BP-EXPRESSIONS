CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -O0
INCLUDES := -Iautomaton -Iparser -Itokenizer -Iutils

SRC_DIRS := automaton parser tokenizer utils .
BUILD    := build

SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))

TARGET := expressions

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET)

INPUT := "a,a,b,b,c,c"
run: all
	./$(TARGET) json-examples/1.1-HZA.json $(INPUT)

.PHONY: all clean run