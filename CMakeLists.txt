cmake_minimum_required(VERSION 3.19)
project(REminiCRT)

find_package(SDL2 REQUIRED)

add_definitions(-DUSE_MODPLUG -DUSE_ZLIB)

find_path(
        SDL_GPU_INCLUDE_DIR
        NAMES SDL_gpu.h
        PATHS /usr/local/include /Users/bni/Documents/sdl-gpu/include
)

find_path(
        MODPLUG_INCLUDE_DIR
        NAMES libmodplug/modplug.h
        PATHS /usr/local/include /opt/local/include
)

include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${SDL_GPU_INCLUDE_DIR}
        ${MODPLUG_INCLUDE_DIR}
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
