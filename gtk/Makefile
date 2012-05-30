TARGET := umusc
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
HEADERS := $(wildcard *.hpp)

PREFIX = /usr/local

CXX := g++
CXXFLAGS += -O3 -g -std=gnu++0x -Wall -pedantic $(shell pkg-config gtkmm-2.4 --cflags)
LDFLAGS += $(shell pkg-config gtkmm-2.4 --libs)

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
	install -m644 umusc.png /usr/share/icons
	install -m644 umusc.desktop /usr/share/applications

.PHONY: clean

