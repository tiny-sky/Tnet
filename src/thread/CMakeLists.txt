add_library(
    Tnet_thread
    OBJECT
    curthread.cc
    eventloopthread.cc
    eventloopthreadpool.cc
    thread.cc)

target_include_directories(Tnet_thread PUBLIC ${PROJECT_SOURCE_DIR}/src/include/thread)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:Tnet_thread>
    PARENT_SCOPE)
