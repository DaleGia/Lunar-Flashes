CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -g

TARGET := image-unpacker

SRCS := main.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)