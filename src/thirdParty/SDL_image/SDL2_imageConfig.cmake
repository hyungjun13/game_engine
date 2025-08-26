
# SDL2_imageConfig.cmake - Cross‑platform configuration for SDL2_image

# Determine the root directory for SDL2_image (the directory containing this file)
get_filename_component(SDL2_IMAGE_ROOT "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

if(APPLE)
  # On macOS, assume SDL2_image is provided as a Framework.
  set(SDL2_IMAGE_FRAMEWORK_PATH "${SDL2_IMAGE_ROOT}/lib/SDL2_image.framework")
  if(NOT EXISTS "${SDL2_IMAGE_FRAMEWORK_PATH}")
    message(FATAL_ERROR "SDL2_image.framework not found at: ${SDL2_IMAGE_FRAMEWORK_PATH}")
  endif()

  # Framework headers live in Headers/
  set(SDL2_IMAGE_INCLUDE_DIR "${SDL2_IMAGE_FRAMEWORK_PATH}/Headers")
  # The binary is inside the framework bundle
  set(SDL2_IMAGE_LIBRARY  "${SDL2_IMAGE_FRAMEWORK_PATH}/SDL2_image")

  add_library(SDL2_image::SDL2_image SHARED IMPORTED)
  set_target_properties(SDL2_image::SDL2_image PROPERTIES
    IMPORTED_LOCATION        "${SDL2_IMAGE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIR}"
    FRAMEWORK                TRUE
  )

elseif(WIN32)
  # On Windows, use the .lib file
  set(SDL2_IMAGE_INCLUDE_DIR "${SDL2_IMAGE_ROOT}")
  set(SDL2_IMAGE_LIBRARY     "${SDL2_IMAGE_ROOT}/lib/SDL2_image.lib")
  if(NOT EXISTS "${SDL2_IMAGE_LIBRARY}")
    message(FATAL_ERROR "SDL2_image.lib not found at: ${SDL2_IMAGE_LIBRARY}")
  endif()

  add_library(SDL2_image::SDL2_image STATIC IMPORTED)
  set_target_properties(SDL2_image::SDL2_image PROPERTIES
    IMPORTED_LOCATION        "${SDL2_IMAGE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIR}"
  )

else()
  # On Linux/others, assume static .a with standard suffix
  set(SDL2_IMAGE_INCLUDE_DIR "${SDL2_IMAGE_ROOT}/include")
  set(SDL2_IMAGE_LIB_SUFFIX   "${CMAKE_STATIC_LIBRARY_SUFFIX}")  # e.g. ".a"
  set(SDL2_IMAGE_LIBRARY      "${SDL2_IMAGE_ROOT}/lib/SDL2_image${SDL2_IMAGE_LIB_SUFFIX}")

  if(NOT EXISTS "${SDL2_IMAGE_LIBRARY}")
    message(FATAL_ERROR "libSDL2_image${SDL2_IMAGE_LIB_SUFFIX} not found at: ${SDL2_IMAGE_LIBRARY}")
  endif()

  add_library(SDL2_image::SDL2_image STATIC IMPORTED)
  set_target_properties(SDL2_image::SDL2_image PROPERTIES
    IMPORTED_LOCATION        "${SDL2_IMAGE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIR}"
  )
endif()

# Mark package found
set(SDL2_IMAGE_FOUND TRUE)
