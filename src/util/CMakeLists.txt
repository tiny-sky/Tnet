add_library(
    Tnet_util
    OBJECT
    buffer.cc
    log.cc
    timestamp.cc)

target_include_directories(Tnet_util PUBLIC ${PROJECT_SOURCE_DIR}/src/include/util)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:Tnet_util>
    PARENT_SCOPE)

