# SDL2Config.cmake - Cross‑platform configuration for SDL2 & SDL2main

# Determine the root directory for SDL2 (where this file lives)
get_filename_component(SDL2_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if(APPLE)
  # macOS: use SDL2.framework
  set(FW "${SDL2_ROOT}/lib/SDL2.framework")
  if(NOT EXISTS "${FW}")
    message(FATAL_ERROR "SDL2.framework not found at: ${FW}")
  endif()

  # Headers and binary inside the framework
  set(SDL2_INCLUDE_DIR "${FW}/Headers")
  set(SDL2_LIBRARY     "${FW}/SDL2")

  # Imported target for SDL2.framework
  add_library(SDL2::SDL2 SHARED IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION            "${SDL2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
    FRAMEWORK                    TRUE
  )

  # Alias SDL2main back to SDL2 (framework covers both)
  add_library(SDL2::SDL2main ALIAS SDL2::SDL2)

elseif(WIN32)
  # Windows: .lib files
  set(SDL2_INCLUDE_DIR "${SDL2_ROOT}")
  set(SDL2_LIB_DIR     "${SDL2_ROOT}/lib")

  # SDL2.lib
  set(SDL2_LIBRARY     "${SDL2_LIB_DIR}/SDL2.lib")
  if(NOT EXISTS "${SDL2_LIBRARY}")
    message(FATAL_ERROR "SDL2.lib not found at: ${SDL2_LIBRARY}")
  endif()
  add_library(SDL2::SDL2 STATIC IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION            "${SDL2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
  )

  # SDL2main.lib
  set(SDL2MAIN_LIBRARY "${SDL2_LIB_DIR}/SDL2main.lib")
  if(NOT EXISTS "${SDL2MAIN_LIBRARY}")
    message(FATAL_ERROR "SDL2main.lib not found at: ${SDL2MAIN_LIBRARY}")
  endif()
  add_library(SDL2::SDL2main STATIC IMPORTED)
  set_target_properties(SDL2::SDL2main PROPERTIES
    IMPORTED_LOCATION            "${SDL2MAIN_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
  )

else()
  # Linux/Other: .a static libs
  set(SDL2_INCLUDE_DIR "${SDL2_ROOT}/include")
  set(SDL2_LIB_DIR     "${SDL2_ROOT}/lib")
  set(_SUFFIX          "${CMAKE_STATIC_LIBRARY_SUFFIX}")  # e.g. ".a"

  # libSDL2.a
  set(SDL2_LIBRARY      "${SDL2_LIB_DIR}/SDL2${_SUFFIX}")
  if(NOT EXISTS "${SDL2_LIBRARY}")
    message(FATAL_ERROR "libSDL2${_SUFFIX} not found at: ${SDL2_LIBRARY}")
  endif()
  add_library(SDL2::SDL2 STATIC IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION            "${SDL2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
  )

  # libSDL2main.a
  set(SDL2MAIN_LIBRARY  "${SDL2_LIB_DIR}/SDL2main${_SUFFIX}")
  if(NOT EXISTS "${SDL2MAIN_LIBRARY}")
    message(FATAL_ERROR "libSDL2main${_SUFFIX} not found at: ${SDL2MAIN_LIBRARY}")
  endif()
  add_library(SDL2::SDL2main STATIC IMPORTED)
  set_target_properties(SDL2::SDL2main PROPERTIES
    IMPORTED_LOCATION            "${SDL2MAIN_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
  )
endif()

# Mark package found
set(SDL2_FOUND TRUE)
