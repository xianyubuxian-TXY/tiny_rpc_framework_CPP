cmake_minimum_required(VERSION 3.16)
project(MyProject   #项目名
    VERSION 1.0.0 	#项目版本号
    LANGUAGES CXX)	# 项目所用语言

# 2.1 编译选项/标准
set(CMAKE_CXX_STANDARD 17)	# 指定使用 C++17 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON) #强制要求编译器支持 C++17，若编译器不支持，直接报错
set(CMAKE_CXX_EXTENSIONS OFF) #禁用编译器扩展语法， 保证代码在不同编译器下行为一致
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 2.2 默认构建类型（多配置生成器如 MSVC / Xcode 会忽略这个）
# 核心逻辑：若用户未指定 CMAKE_BUILD_TYPE（如编译时未加 -DCMAKE_BUILD_TYPE=Debug），
# 默认设置为 Release（优化编译，去除调试信息，适合部署）；
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

# 2.3 一些开关（可在 cmake-gui / ccmake 里改）
option(BUILD_TESTS "Build unit tests" ON) # 控制是否编译单元测试代码（tests 子目录）
# 控制生成 “动态库（.so/.dll）” 还是 “静态库（.a/.lib）”，默认 OFF 即生成静态库；
option(BUILD_SHARED_LIBS "Build shared instead of static libs" OFF) 

# 2.4 把 cmake/ 目录加到模块路径（放自定义 FindXXX.cmake）
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 2.5 寻找库包 
find_package(Protobuf REQUIRED)

# 2.6 子目录
add_subdirectory(src)
add_subdirectory(examples)