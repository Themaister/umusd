TARGET := umusd
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
HEADERS := $(wildcard *.hpp)

CXX := g++
CXXFLAGS += -O0 -g -std=c++11 -Wall -pedantic $(shell pkg-config libavutil libavformat libavcodec --cflags)
LDFLAGS += $(shell pkg-config libavutil libavformat libavcodec --libs)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)

.PHONY: clean

