CC = gcc
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include

TARGET = main
SRC = main.cc ./include/DBUtil/DatabaseUtil.cc ./include/LogUtil/LogUtil.cc ./include/FtpUtil/ftplib.cc

# dynamic library path
LIBS = -L./lib -lpthread -lmysqlcppconn

# xxx.o
OBJS = main.o DatabaseUtil.o LogUtil.o FtpUtil.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $(TARGET) $(CXXFLAGS)
	mv $(OBJS) ./build

# compile c++ file
main.o: main.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

DatabaseUtil.o: ./include/DBUtil/DatabaseUtil.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

LogUtil.o: ./include/LogUtil/LogUtil.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

FtpUtil.o: ./include/FtpUtil/ftplib.cc
	$(CXX) $(CXXFLAGS) -D_REENTRANT -DNOSSL -c $< -o $@

# # compile c file
# cJSON.o: ./include/json/cJSON.c
# 	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -r -f ./build/* ./main