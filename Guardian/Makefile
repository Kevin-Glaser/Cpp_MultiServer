CC = gcc
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include

TARGET = guardian
SRC = guardian.cc ./include/SocketManagerUtil/SocketManager.cc

LIBS = -lpthread -lrt

OBJS = guardian.o SocketManager.o
OBJ_DIR = build

$(shell mkdir -p $(OBJ_DIR))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $(TARGET) $(CXXFLAGS)
	mv $(OBJS) $(OBJ_DIR)

SocketManager.o: ./include/SocketManagerUtil/SocketManager.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

guardian.o: guardian.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) guardian