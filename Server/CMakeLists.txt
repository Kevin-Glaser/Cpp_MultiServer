cmake_minimum_required(VERSION 3.10)
project(Server)

# 源文件
set(SOURCES
    server.cc
    include/LogUtil/LogUtil.cc
    include/ServerUtil/ServerUtil.cc
    include/ConfigUtil/ConfigUtil.cc
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 编译选项和定义
target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_compile_definitions(${PROJECT_NAME} PRIVATE _REENTRANT NOSSL)

# 查找线程库
find_package(Threads REQUIRED)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Threads::Threads
)

# 平台特定配置
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE rt)
endif()