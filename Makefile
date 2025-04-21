CXX      = clang++
CXXFLAGS = -std=c++17 -O3 \
           -Isrc/thirdParty/glm \
           -Isrc/firstParty \
           -Isrc/thirdParty/rapidjson/include \
           -Isrc/thirdParty/SDL2 \
           -Isrc/thirdParty/SDL2_mixer \
           -Isrc/thirdParty/SDL_image \
           -Isrc/thirdParty/SDL2_ttf \
           -Isrc/thirdParty \
           -Isrc/thirdParty/lua \
           -Isrc/thirdParty/LuaBridge

# Link flags (library search paths) + rpaths
# Using $$ORIGIN (double-dollar to defer evaluation by make) without equals or quotes.
LDFLAGS  = \
    -Lsrc/thirdParty/SDL2/lib       -Wl,-rpath,$$ORIGIN/src/thirdParty/SDL2/lib \
    -Lsrc/thirdParty/SDL2_mixer/lib -Wl,-rpath,$$ORIGIN/src/thirdParty/SDL2_mixer/lib \
    -Lsrc/thirdParty/SDL_image/lib  -Wl,-rpath,$$ORIGIN/src/thirdParty/SDL_image/lib \
    -Lsrc/thirdParty/SDL2_ttf/lib   -Wl,-rpath,$$ORIGIN/src/thirdParty/SDL2_ttf/lib

# Actual libraries to link
LDLIBS   = -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -llua5.4

SOURCES  = $(wildcard src/firstParty/*.cpp)
OBJECTS  = $(SOURCES:.cpp=.o)
TARGET   = game_engine_linux

all: clean $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
