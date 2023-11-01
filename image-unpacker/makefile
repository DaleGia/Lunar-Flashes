CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -g

TARGET := FitsLunarFlashesUnpacker

SRCS := fits.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)