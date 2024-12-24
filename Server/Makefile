CC = gcc
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include

TARGET = server
SRC = server.cc ./include/DBUtil/DatabaseUtil.cc ./include/LogUtil/LogUtil.cc ./include/FtpUtil/ftplib.cc

# dynamic library path
LIBS = -L./lib -lpthread -lmysqlcppconn

# xxx.o
OBJS = server.o DatabaseUtil.o LogUtil.o FtpUtil.o DatabasePoolUtil.o
OBJ_DIR = build

$(shell mkdir -p $(OBJ_DIR))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $(TARGET) $(CXXFLAGS)
	mv $(OBJS) $(OBJ_DIR)

# compile c++ file
main.o: server.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

DatabaseUtil.o: ./include/DBUtil/DatabaseUtil.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

DatabasePoolUtil.o: ./include/DBUtil/DatabasePoolUtil.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

LogUtil.o: ./include/LogUtil/LogUtil.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

FtpUtil.o: ./include/FtpUtil/ftplib.cc
	$(CXX) $(CXXFLAGS) -D_REENTRANT -DNOSSL -c $< -o $@

# # compile c file
# cJSON.o: ./include/json/cJSON.c
# 	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -r -f $(OBJ_DIR) ./server