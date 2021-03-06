CC=g++
CFLAGS=-g -c -Wall -std=c++11
LDFLAGS=
SOURCES=pong_test.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=pong_test

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) -g $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(EXECUTABLE)
	find . -type f -name '*.o' -delete
