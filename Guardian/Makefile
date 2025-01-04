CC = gcc
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include

TARGET = guardian
SRC = guardian.cc

LIBS = -lpthread -lrt

OBJS = guardian.o

OBJ_DIR = build

$(shell mkdir -p $(OBJ_DIR))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $(TARGET)
	mv $(OBJS) $(OBJ_DIR)

guardian.o: guardian.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) guardian