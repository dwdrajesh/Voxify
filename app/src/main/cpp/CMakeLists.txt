cmake_minimum_required(VERSION 3.22.1)
project("voxify")

find_package(oboe REQUIRED CONFIG)

add_library( # Specifies the name of the library. (will be libvoxify.so)
        voxify
         # Sets the library as a shared library.
        SHARED

        # Provides a relative path to source file(s).
        jni-bridge.cpp
#        AudioEngine.cpp
)

# Links your native library against one or more other native libraries.
target_link_libraries(
        # Specifies the target library.
        voxify

        # links the oboe, android and log libraries to voxify
        oboe::oboe
        android
        log
)
