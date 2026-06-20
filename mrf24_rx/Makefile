CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I./src
LDFLAGS = 

SRCS = src/main.cpp src/mrf24j40.cpp
OBJS = $(SRCS:src/%.cpp=obj/%.o)
TARGET = bin/mrf24_receiver

all: directories $(TARGET)

directories:
	mkdir -p obj bin

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj bin

.PHONY: all clean directories
