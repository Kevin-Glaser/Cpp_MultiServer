# cmake_minimum_required(VERSION 3.28)
# project(Guardian)

# # 设置C++标准和编译选项
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# add_compile_options(-Wall)

# # 查找Threads包
# find_package(Threads REQUIRED)

# # 明确指定源文件，与Makefile保持一致
# set(GUARDIAN_SOURCES
#     guardian.cc
#     include/SocketManagerUtil/SocketManager.cc
#     include/SharedMemoryUtil/SharedMemoryUtil.cc
#     include/ProcessMonitorUtil/ServerMonitor.cc
# )

# # Guardian可执行文件
# add_executable(Guardian ${GUARDIAN_SOURCES})

# # 设置包含目录
# target_include_directories(Guardian
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/include
# )

# # 链接必要的库
# target_link_libraries(Guardian
#     PRIVATE
#         pthread
# )

# # 平台特定配置
# if(UNIX)
#     target_link_libraries(Guardian
#         PRIVATE
#             rt
#     )
# endif()

# # 安装规则
# install(TARGETS Guardian
#     RUNTIME DESTINATION bin
# )

# # 安装头文件
# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
#     DESTINATION include
#     FILES_MATCHING PATTERN "*.h"
# )

cmake_minimum_required(VERSION 3.10)
project(Guardian)

# 源文件
set(GUARDIAN_SOURCES
    guardian.cc
    include/SocketManagerUtil/SocketManager.cc
    include/SharedMemoryUtil/SharedMemoryUtil.cc
    include/ProcessMonitorUtil/ServerMonitor.cc
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${GUARDIAN_SOURCES})

# 包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

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

# 安装规则
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)