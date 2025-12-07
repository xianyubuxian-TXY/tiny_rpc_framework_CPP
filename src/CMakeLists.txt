set(SOURCES_CODE rpc/rpc_meta.pb.cc
                 rpc/rpc_codec.cc
                 rpc/rpc_dispatcher.cc
                 rpc/rpc_channel.cc
                 net_muduo/muduo_network_server.cc)

add_library(tiny_rpc ${SOURCES_CODE})

set_target_properties(tiny_rpc PROPERTIES
    # 静态库输出路径
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
    # 动态库输出路径（若开启 BUILD_SHARED_LIBS=ON）
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

target_include_directories(tiny_rpc
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
            ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(tiny_rpc
        PUBLIC
            ${Protobuf_LIBRARIES}
            muduo_net
            muduo_base
            pthread
)