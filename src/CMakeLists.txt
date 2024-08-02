add_subdirectory(event)
add_subdirectory(tcpserver)
add_subdirectory(thread)
add_subdirectory(util)

add_library(Tnet STATIC ${ALL_OBJECT_FILES})

set(TNET_LIBS
        Tnet_event
        Tnet_tcpserver
        Tnet_thread
        Tnet_util
        )

find_package(Threads REQUIRED)

set(TNET_THIRDPARTY_LIBS fmt spdlog)

target_link_libraries(
        Tnet
        ${TNET_LIBS}
        ${TNET_THIRDPARTY_LIBS})

target_include_directories(Tnet PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
