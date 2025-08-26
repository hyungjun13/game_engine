# SDL2_ttfConfig.cmake - Cross‑platform configuration for SDL2_ttf

# Determine the root directory for SDL2_ttf (where this file lives)
get_filename_component(SDL2_TTF_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if(APPLE)
  # On macOS, use the framework
  set(SDL2_TTF_FRAMEWORK_PATH "${SDL2_TTF_ROOT}/lib/SDL2_ttf.framework")
  if(NOT EXISTS "${SDL2_TTF_FRAMEWORK_PATH}")
    message(FATAL_ERROR "SDL2_ttf.framework not found at: ${SDL2_TTF_FRAMEWORK_PATH}")
  endif()

  set(SDL2_TTF_INCLUDE_DIR "${SDL2_TTF_FRAMEWORK_PATH}/Headers")
  set(SDL2_TTF_LIBRARY     "${SDL2_TTF_FRAMEWORK_PATH}/SDL2_ttf")

  add_library(SDL2_ttf::SDL2_ttf SHARED IMPORTED)
  set_target_properties(SDL2_ttf::SDL2_ttf PROPERTIES
    IMPORTED_LOCATION           "${SDL2_TTF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIR}"
    FRAMEWORK                   TRUE
  )

elseif(WIN32)
  # On Windows, use the .lib file
  set(SDL2_TTF_INCLUDE_DIR "${SDL2_TTF_ROOT}")
  set(SDL2_TTF_LIBRARY     "${SDL2_TTF_ROOT}/lib/SDL2_ttf.lib")
  if(NOT EXISTS "${SDL2_TTF_LIBRARY}")
    message(FATAL_ERROR "SDL2_ttf.lib not found at: ${SDL2_TTF_LIBRARY}")
  endif()

  add_library(SDL2_ttf::SDL2_ttf STATIC IMPORTED)
  set_target_properties(SDL2_ttf::SDL2_ttf PROPERTIES
    IMPORTED_LOCATION           "${SDL2_TTF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIR}"
  )

else()
  # On Linux/other, assume static .a
  set(SDL2_TTF_INCLUDE_DIR "${SDL2_TTF_ROOT}/include")
  set(SDL2_TTF_LIB_SUFFIX   "${CMAKE_STATIC_LIBRARY_SUFFIX}")  # e.g. ".a"
  set(SDL2_TTF_LIBRARY      "${SDL2_TTF_ROOT}/lib/SDL2_ttf${SDL2_TTF_LIB_SUFFIX}")

  if(NOT EXISTS "${SDL2_TTF_LIBRARY}")
    message(FATAL_ERROR "libSDL2_ttf${SDL2_TTF_LIB_SUFFIX} not found at: ${SDL2_TTF_LIBRARY}")
  endif()

  add_library(SDL2_ttf::SDL2_ttf STATIC IMPORTED)
  set_target_properties(SDL2_ttf::SDL2_ttf PROPERTIES
    IMPORTED_LOCATION           "${SDL2_TTF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIR}"
  )
endif()

# Mark package found
set(SDL2_TTF_FOUND TRUE)
