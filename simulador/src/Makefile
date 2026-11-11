CXX      = g++
INC_DIR  = ../Include
BUILD    = build
TARGET   = calles

SRCS     = $(shell find . -name '*.cpp' -not -path './$(BUILD)/*')
OBJS     = $(SRCS:./%.cpp=$(BUILD)/%.o)
DEPS     = $(OBJS:.o=.d)
INCLUDES = $(addprefix -I,$(shell find $(INC_DIR) -type d))

CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -MMD -MP $(INCLUDES)

# En Windows el ejecutable termina en .exe
ifeq ($(OS),Windows_NT)
    TARGET = calles.exe
endif

all: $(TARGET)

# make install hace lo mismo que make: compila el programa
install: all

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)

.PHONY: all install run clean

-include $(DEPS)