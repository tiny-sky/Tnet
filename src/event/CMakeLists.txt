add_library(
    Tnet_event
    OBJECT
    channel.cc
    epoller.cc
    eventloop.cc
    poller.cc
)

target_include_directories(Tnet_event PUBLIC ${PROJECT_SOURCE_DIR}/src/include/event)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:Tnet_event>
    PARENT_SCOPE)