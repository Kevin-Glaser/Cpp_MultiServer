CC = gcc
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include

TARGET = guardian

LIBS = -lpthread -lrt

OBJS = guardian.o SocketManager.o SharedMemoryUtil.o ServerMonitor.o
OBJ_DIR = build

$(shell mkdir -p $(OBJ_DIR))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $(TARGET) $(CXXFLAGS)
	mv $(OBJS) $(OBJ_DIR)

SocketManager.o: ./include/SocketManagerUtil/SocketManager.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

SharedMemoryUtil.o: ./include/SharedMemoryUtil/SharedMemoryUtil.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

ServerMonitor.o: ./include/ProcessMonitorUtil/ServerMonitor.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

guardian.o: guardian.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) guardian