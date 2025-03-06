cmake_minimum_required(VERSION 3.10)
project(CPP_Server
  VERSION 1.0
  LANGUAGES CXX
  DESCRIPTION "Cross-platform Server and Guardian Project"
)

# 全局设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 跨平台编译器选项
if(MSVC)
  add_compile_options(/W4 /WX -Wno-error=unused-parameter)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-error=unused-parameter)
endif()

# 设置输出目录（区分平台）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含子项目
add_subdirectory(Server)
add_subdirectory(Guardian)