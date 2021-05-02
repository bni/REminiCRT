
SDL_CFLAGS   := `sdl2-config --cflags`
SDL_LIBS     := `sdl2-config --libs`

GPU_CFLAGS := -I/Users/bni/Documents/sdl-gpu/include/SDL2 -D_THREAD_SAFE
GPU_LIBS := -L/Users/bni/Documents/sdl-gpu/lib -lSDL2_gpu

MODPLUG_LIBS := -lmodplug
ZLIB_LIBS    := -lz

CXXFLAGS += -Wall -Wpedantic -Wno-newline-eof -MMD $(SDL_CFLAGS) $(GPU_CFLAGS) -I/opt/local/include -DUSE_MODPLUG -DUSE_ZLIB

SRCS = collision.cpp cutscene.cpp file.cpp fs.cpp game.cpp graphics.cpp main.cpp \
	menu.cpp mixer.cpp mod_player.cpp piege.cpp protection.cpp resource.cpp \
	sfx_player.cpp staticres.cpp systemstub_sdl.cpp unpack.cpp util.cpp video.cpp


OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

LIBS = $(SDL_LIBS) $(GPU_LIBS) $(MODPLUG_LIBS) $(ZLIB_LIBS)

LDFLAGS= -framework GLUT -framework OpenGL -framework Cocoa

COPY_FILES = $(BUILD_DIR)/rs $(BUILD_DIR)/rs.cfg

rs: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(DEPS)

app:
	@cp rs Flashback.app/Contents/MacOS/
	@cp /opt/local/lib/libSDL2-2.0.0.dylib Flashback.app/Contents/libs/
	@cp /opt/local/lib/libmodplug.1.dylib Flashback.app/Contents/libs/
	@cp /opt/local/lib/libz.1.dylib Flashback.app/Contents/libs/
	@cp rs.cfg Flashback.app/Contents/Resources/
	@cp vertex.shader Flashback.app/Contents/Resources/
	@cp pixel.shader Flashback.app/Contents/Resources/
	@cp -r DATA Flashback.app/Contents/Resources/
	#@otool -L Flashback.app/Contents/MacOS/rs
	@../macdylibbundler/dylibbundler -od -b -x Flashback.app/Contents/MacOS/rs
	#@otool -L Flashback.app/Contents/MacOS/rs

-include $(DEPS)
