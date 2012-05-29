TARGET := umusc
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
HEADERS := $(wildcard *.hpp)

CXX := g++
CXXFLAGS += -O3 -g -std=c++11 -Wall -pedantic $(shell pkg-config gtkmm-2.4 --cflags)
LDFLAGS += $(shell pkg-config gtkmm-2.4 --libs)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)

.PHONY: clean
