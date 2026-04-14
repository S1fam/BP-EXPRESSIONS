# ===== compiler =====
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Icore -Iparser -Itokenizer -Iutils

# ===== target =====
TARGET = parser_app

# ===== source files =====
SRCS = \
    main.cpp \
    core/display_welcome.cpp \
    core/recieve_expression.cpp \
    parser/parser.cpp \
    tokenizer/tokenizer.cpp \
    utils/automaton.cpp

# ===== object files =====
OBJS = $(SRCS:.cpp=.o)

# ===== build =====
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

# ===== compilation rule =====
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ===== run =====
run: $(TARGET)
	./$(TARGET)

# ===== clean =====
clean:
	rm -f $(OBJS) $(TARGET)