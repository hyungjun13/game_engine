cmake_minimum_required(VERSION 3.10)
project(GameEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD       17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS     OFF)

# Tell CMake to look in vendored cmake-config dirs first.
list(APPEND CMAKE_PREFIX_PATH
  "${CMAKE_SOURCE_DIR}/src/thirdParty/SDL2"
  "${CMAKE_SOURCE_DIR}/src/thirdParty/SDL_image"
  "${CMAKE_SOURCE_DIR}/src/thirdParty/SDL2_ttf"
  "${CMAKE_SOURCE_DIR}/src/thirdParty/SDL2_mixer"
)

# 1) Build Lua
add_subdirectory(src/thirdParty/lua)

if(APPLE)
  # Homebrew config packages are commonly installed here on Apple Silicon.
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

# 2) Find all of our SDL2 & friends via the provided SDL2Config.cmake files
find_package(SDL2       CONFIG REQUIRED)    # provides SDL2::SDL2 & SDL2::SDL2main
find_package(SDL2_image CONFIG REQUIRED)    # provides SDL2_image::SDL2_image
find_package(SDL2_ttf   CONFIG REQUIRED)    # provides SDL2_ttf::SDL2_ttf
find_package(SDL2_mixer CONFIG REQUIRED)    # provides SDL2_mixer::SDL2_mixer

# 3) Collect engine sources
file(GLOB_RECURSE
  GAME_ENGINE_SOURCES
  CONFIGURE_DEPENDS
    src/firstParty/*.cpp
    src/firstParty/*.hpp
)

# 3b) Build Box2D from vendored source
file(GLOB_RECURSE BOX2D_SOURCES
  src/thirdParty/box2d/src/*.cpp
)

add_library(box2d STATIC ${BOX2D_SOURCES})
target_include_directories(box2d
  PUBLIC
    src/thirdParty/box2d/include
  PRIVATE
    src/thirdParty/box2d/src
)
set_target_properties(box2d PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

# 4) Create executable
add_executable(${PROJECT_NAME} ${GAME_ENGINE_SOURCES})

# 5) Include your own headers + LuaBridge, GLM, RapidJSON
target_include_directories(${PROJECT_NAME} PRIVATE
    src/firstParty
    src/thirdParty            # for <LuaBridge/...>, <rapidjson/...>
    src/thirdParty/glm
    src/thirdParty/LuaBridge
    src/thirdParty/lua        # for Lua’s headers
)

# Treat RapidJSON as a system dependency so third-party deprecation noise
# (e.g. std::iterator in C++17 mode) does not pollute project warnings.
target_include_directories(${PROJECT_NAME} SYSTEM BEFORE PRIVATE
  src/thirdParty/rapidjson/include
)

# 6) Compiler flags
if (MSVC)
    # /W4 (warning level) in all configs; /O2 only in Release
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/W4>
        $<$<CONFIG:Release>:/W4;/O2>
    )
else()
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -O3
    -Wno-deprecated-declarations
  )
endif()

# On macOS ensure we link libc++
if(APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE "-stdlib=libc++")
    target_link_options   (${PROJECT_NAME} PRIVATE "-stdlib=libc++")
endif()

# 7) Link everything
target_link_libraries(${PROJECT_NAME} PRIVATE
    lua                          # your static Lua
  box2d
    SDL2::SDL2
    SDL2::SDL2main               # alias to SDL2 framework / import target
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
)

# 8) (Optional) Copy resources, set RPATH, etc.
#    … your existing post‐build resource copy steps …
if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Make sure the destination folder exists
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    # Copy every DLL from your repo’s `dlls/` dir
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${PROJECT_SOURCE_DIR}/dlls"
      "$<TARGET_FILE_DIR:${PROJECT_NAME}>"

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources" 
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
    
  )
endif()
