add_executable(torpedo
    player.cpp
    torpedo.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/../seasocks/build/src/main/c)

target_link_libraries(torpedo
    ${CMAKE_SOURCE_DIR}/../seasocks/build/src/main/c/libseasocks.a
    ${ZLIB_LIBRARIES}
)
