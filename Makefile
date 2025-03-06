# Description: 项目构建脚本
# 编译器配置
CC       = gcc
CXX      = g++
CXXFLAGS = -std=c++17 -Wall \
           -IServer/include \
           -IGuardian/include

# 路径配置
BUILD_DIR   = build
OBJ_DIR     = $(BUILD_DIR)/obj
BIN_DIR     = $(BUILD_DIR)/bin

# 目标配置
SERVER_TARGET   = $(BIN_DIR)/Server
GUARDIAN_TARGET = $(BIN_DIR)/Guardian

# Server 源文件列表（自动递归查找）
SERVER_SRCS := $(shell find Server -name '*.cc')

# Guardian 源文件列表（自动递归查找）
GUARDIAN_SRCS := $(shell find Guardian -name '*.cc')

# 对象文件生成规则
SERVER_OBJS   = $(patsubst Server/%, $(OBJ_DIR)/Server/%.o, $(basename $(SERVER_SRCS)))
GUARDIAN_OBJS = $(patsubst Guardian/%, $(OBJ_DIR)/Guardian/%.o, $(basename $(GUARDIAN_SRCS)))

# 链接库配置
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
    LIBS = -lpthread -lrt
else
    LIBS = -lpthread
endif

# 构建规则
all: prepare $(SERVER_TARGET) $(GUARDIAN_TARGET)

prepare:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)/Server $(OBJ_DIR)/Guardian

# Server 可执行文件
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $^ $(LIBS) -o $@ $(CXXFLAGS)

# Guardian 可执行文件
$(GUARDIAN_TARGET): $(GUARDIAN_OBJS)
	$(CXX) $^ $(LIBS) -o $@ $(CXXFLAGS)

# 通用编译规则
$(OBJ_DIR)/Server/%.o: Server/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/Guardian/%.o: Guardian/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理规则
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all prepare clean