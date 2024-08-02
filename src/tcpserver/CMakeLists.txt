add_library(
    Tnet_tcpserver
    OBJECT
    acceptor.cc
    inetaddress.cc
    socket.cc
    tcpconnection.cc
    tcpserver.cc)

target_include_directories(Tnet_tcpserver PUBLIC ${PROJECT_SOURCE_DIR}/src/include/tcpserver)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:Tnet_tcpserver>
    PARENT_SCOPE)
