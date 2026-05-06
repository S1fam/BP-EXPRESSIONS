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

run: all
	./$(TARGET)

.PHONY: all clean run