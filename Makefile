CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
TARGET   := parser_app
SRCS     := main.cpp automaton.cpp parser.cpp tokenizer.cpp \
            display_welcome.cpp recieve_expression.cpp
OBJS     := $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)