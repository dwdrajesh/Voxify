cmake_minimum_required(VERSION 3.22.1)
project("voxify")

find_package(oboe REQUIRED CONFIG)

# Build whisper.cpp as a subdirectory
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # do not build examples, tests etc, only need library
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(whisper.cpp)

add_library( # Specifies the name of the library. (will be libvoxify.so)
        voxify
         # Sets the library as a shared library.
        SHARED

        # Provides a relative path to source file(s).
        jni-bridge.cpp
        AudioEngine.cpp
)

target_include_directories(voxify PRIVATE # so our AudioEngine.cpp can #include "whisper.h"
        whisper.cpp/include
        whisper.cpp/ggml/include
)

# Links your native library against one or more other native libraries.
target_link_libraries(
        # Specifies the target library.
        voxify

        # links the oboe, android and log libraries to voxify
        oboe::oboe
        whisper # for translation
        android
        log
)
