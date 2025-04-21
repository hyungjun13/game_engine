# SDL2_mixerConfig.cmake - Cross‑platform configuration for SDL2_mixer

# Determine the root directory for SDL2_mixer (the directory containing this file)
get_filename_component(SDL2_MIXER_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if(APPLE)
  # On macOS, assume SDL2_mixer is provided as a Framework.
  set(SDL2_MIXER_FRAMEWORK_PATH "${SDL2_MIXER_ROOT}/lib/SDL2_mixer.framework")
  if(NOT EXISTS "${SDL2_MIXER_FRAMEWORK_PATH}")
    message(FATAL_ERROR "SDL2_mixer.framework not found at: ${SDL2_MIXER_FRAMEWORK_PATH}")
  endif()

  # Framework headers live in Headers/
  set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_FRAMEWORK_PATH}/Headers")
  # The binary is inside the framework bundle
  set(SDL2_MIXER_LIBRARY  "${SDL2_MIXER_FRAMEWORK_PATH}/SDL2_mixer")

  add_library(SDL2_mixer::SDL2_mixer SHARED IMPORTED)
  set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
    IMPORTED_LOCATION           "${SDL2_MIXER_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_INCLUDE_DIR}"
    FRAMEWORK                   TRUE
  )

elseif(WIN32)
  # On Windows, use the .lib file
  set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_ROOT}")
  set(SDL2_MIXER_LIBRARY     "${SDL2_MIXER_ROOT}/lib/SDL2_mixer.lib")
  if(NOT EXISTS "${SDL2_MIXER_LIBRARY}")
    message(FATAL_ERROR "SDL2_mixer.lib not found at: ${SDL2_MIXER_LIBRARY}")
  endif()

  add_library(SDL2_mixer::SDL2_mixer STATIC IMPORTED)
  set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
    IMPORTED_LOCATION           "${SDL2_MIXER_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_INCLUDE_DIR}"
  )

else()
  # On Linux/others, assume static .a with standard suffix
  set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_ROOT}/include")
  set(SDL2_MIXER_LIB_SUFFIX   "${CMAKE_STATIC_LIBRARY_SUFFIX}")  # e.g. ".a"
  set(SDL2_MIXER_LIBRARY      "${SDL2_MIXER_ROOT}/lib/SDL2_mixer${SDL2_MIXER_LIB_SUFFIX}")

  if(NOT EXISTS "${SDL2_MIXER_LIBRARY}")
    message(FATAL_ERROR "libSDL2_mixer${SDL2_MIXER_LIB_SUFFIX} not found at: ${SDL2_MIXER_LIBRARY}")
  endif()

  add_library(SDL2_mixer::SDL2_mixer STATIC IMPORTED)
  set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
    IMPORTED_LOCATION           "${SDL2_MIXER_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_INCLUDE_DIR}"
  )
endif()

# Mark package as found
set(SDL2_MIXER_FOUND TRUE)
