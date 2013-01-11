TARGET := umusd
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
HEADERS := $(wildcard *.hpp)

PREFIX = /usr/local

CXX := g++
CXXFLAGS += -std=gnu++0x -Wall -pedantic $(shell pkg-config libavutil libavformat libavcodec alsa --cflags)
CXXFLAGS += -D__STDC_CONSTANT_MACROS
LDFLAGS += $(shell pkg-config libavutil libavformat libavcodec alsa --libs)

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
else
   CXXFLAGS += -O3
endif

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

