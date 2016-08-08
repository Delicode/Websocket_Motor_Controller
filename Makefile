CC=g++
CFLAGS=-std=c++11 -g -I/home/pi/websocketpp-master/
LDFLAGS=-L/home/pi/boost_1_61_0/stage/lib -L/usr/local/lib -Wall -lboost_system -lboost_random -lboost_thread -lpthread -lboost_timer -lboost_chrono -lrt

SOURCES=$(wildcard ./*.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=websocket_motor

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

clean:
	rm *.o

