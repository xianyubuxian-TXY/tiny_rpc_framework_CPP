cmake_minimum_required(VERSION 3.10)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 生成 echo.pb.{h,cc}
protobuf_generate_cpp(ECHO_PROTO_SRCS ECHO_PROTO_HDRS ${PROJECT_SOURCE_DIR}/proto/echo.proto)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}     # 当前 CMakeLists.txt 所在的「源码目录」 (当前路径)
    ${CMAKE_CURRENT_BINARY_DIR}     # 当前 CMakeLists.txt 对应的「构建目录  （build）
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(echo_server
    echo.pb.cc
    echo_server_main.cc
    ${ECHO_PROTO_SRCS}
)

# 1. 指定tiny_rpc库的搜索路径（对应你生成libtiny_rpc.a的目录）
link_directories(${PROJECT_SOURCE_DIR}/lib)

# 2. 链接tiny_rpc（替代错误的rpc_net_muduo/rpc_core），保留其他必要依赖
target_link_libraries(echo_server
    tiny_rpc  # 核心：替换为实际存在的库名
    pthread
    muduo_net
    muduo_base
    ${Protobuf_LIBRARIES}
)

add_executable(echo_client
    echo_client_main.cc
    ${ECHO_PROTO_SRCS}
)

target_link_libraries(echo_client
    tiny_rpc  # 核心：替换为实际存在的库名
    pthread
    muduo_net
    muduo_base
    ${Protobuf_LIBRARIES}
)
