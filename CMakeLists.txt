cmake_minimum_required(VERSION 3.19)
project(REminiCRT)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_definitions(-DUSE_MODPLUG -DUSE_ZLIB)

find_path(
        SDL_GPU_INCLUDE_DIR
        NAMES SDL_gpu.h
        PATHS /usr/local/include ${PROJECT_SOURCE_DIR}/SDL_gpu/include/SDL2
)

find_library(
        SDL_GPU_LIBRARY
        libSDL2_gpu.a
        PATHS ${PROJECT_SOURCE_DIR}/SDL_gpu/lib
)

find_path(
        MODPLUG_INCLUDE_DIR
        NAMES libmodplug/modplug.h
        PATHS /usr/local/include /opt/local/include
)

find_library(
        MODPLUG_LIBRARY
        libmodplug.dylib
)

find_path(
        ZLIB_INCLUDE_DIR
        NAMES zlib.h
        PATHS /usr/local/include /opt/local/include
)

find_library(
        ZLIB_LIBRARY
        libz.dylib
)

include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${SDL_GPU_INCLUDE_DIR}
        ${MODPLUG_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
)

add_executable(
        rs
        collision.cpp
        cutscene.cpp
        file.cpp
        fs.cpp
        game.cpp
        graphics.cpp
        main.cpp
        menu.cpp
        mixer.cpp
        mod_player.cpp
        piege.cpp
        protection.cpp
        resource.cpp
        sfx_player.cpp
        staticres.cpp
        systemstub_sdl.cpp
        unpack.cpp
        util.cpp
        video.cpp
)

set (CMAKE_EXE_LINKER_FLAGS -mmacosx-version-min=15.6)

target_link_libraries(
        rs
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${SDL_GPU_LIBRARY}
        ${MODPLUG_LIBRARY}
        ${ZLIB_LIBRARY}
)

configure_file(rs.cfg rs.cfg COPYONLY)
configure_file(pixel.shader pixel.shader COPYONLY)
configure_file(vertex.shader vertex.shader COPYONLY)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/DATA/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/DATA/)
