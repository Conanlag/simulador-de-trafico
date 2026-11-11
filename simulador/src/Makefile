CXX      = g++
TARGET   = calles
 
SRC      = $(shell find . -type f -name '*.cpp')
HEADERS  = $(shell find ../Include -type f -name '*.h')
INCLUDES = $(addprefix -I,$(shell find ../Include -type d))
 
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -fopenmp $(INCLUDES)
 
INSTALL_DIR = $(HOME)/.local/bin
 
all: $(TARGET)
 
# Depende tambien de los .h: si cambias una cabecera, se vuelve a compilar
$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -pthread -o $(TARGET)
 
run: $(TARGET)
	./$(TARGET)
 
clean:
	rm -f $(TARGET)
 
install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/$(TARGET)
 
uninstall:
	rm -f $(INSTALL_DIR)/$(TARGET)
 
.PHONY: all run clean install uninstall