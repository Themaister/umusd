TARGET := umusd
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
HEADERS := $(wildcard *.hpp)

PREFIX = /usr/local

CXX := g++
CXXFLAGS += -O3 -g -std=gnu++0x -Wall -pedantic $(shell pkg-config libavutil libavformat libavcodec alsa --cflags)
CXXFLAGS += -D__STDC_CONSTANT_MACROS
LDFLAGS += $(shell pkg-config libavutil libavformat libavcodec alsa --libs)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS)

install: all
	install -m755 $(TARGET) $(PREFIX)/bin

.PHONY: clean

