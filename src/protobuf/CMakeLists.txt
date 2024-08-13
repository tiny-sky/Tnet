add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc.proto
  VERBATIM )

add_library(
    Tnet_protobuf
    OBJECT
    ProtobufCodeLite.cc
    rpc.pb.cc
    RpcServer.cc
    RpcChannel.cc)

target_include_directories(Tnet_protobuf PUBLIC ${PROJECT_SOURCE_DIR}/src/include/protobuf)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:Tnet_protobuf>
    PARENT_SCOPE)