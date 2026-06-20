CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I./src -pthread
LDFLAGS  = -pthread -lbcm2835 -lbcm2835

SRCS    = src/main.cpp src/mrf24j40.cpp src/oled.cpp src/font5x7.cpp
OBJS    = $(patsubst src/%.cpp, obj/%.o, $(SRCS))
TARGET  = bin/mrf24_receiver

.PHONY: all clean

all: obj bin $(TARGET)
	@echo ""
	@echo "  Compilación exitosa: $(TARGET)"
	@echo "  Ejecutar con: sudo ./$(TARGET)"

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.cpp src/mrf24j40.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj:
	mkdir -p obj

bin:
	mkdir -p bin

clean:
	rm -rf obj bin
