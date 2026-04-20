cmake_minimum_required(VERSION 3.20)
project(kitty_chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHIP8_ENABLE_IMGUI "Enable Dear ImGui debug UI (vendored)" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(chip8_core
  src/core/Chip8VM.cpp
  src/core/Config.cpp
  src/core/InputBindingsStore.cpp
  src/core/Rom.cpp
)
target_include_directories(chip8_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(chip8_core PUBLIC cxx_std_20)
target_compile_options(chip8_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(chip8_platform_sdl
  src/platform/sdl/SdlPlatform.cpp
)
target_include_directories(chip8_platform_sdl PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(chip8_platform_sdl PUBLIC chip8_core ${SDL2_LIBRARIES})
target_compile_options(chip8_platform_sdl PRIVATE -Wall -Wextra -Wpedantic)

add_executable(chip8
  src/app/main.cpp
  src/app/ui/BitmapFont.cpp
  src/app/ui/RomBrowser.cpp
)
target_link_libraries(chip8 PRIVATE chip8_platform_sdl chip8_core)

add_executable(chip8_config
  src/app/config_main.cpp
  src/app/ui/BitmapFont.cpp
)
target_link_libraries(chip8_config PRIVATE chip8_platform_sdl chip8_core)

# Custom target to clean all generated files including build artifacts
add_custom_target(clean-all
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
  COMMAND ${CMAKE_COMMAND} -E remove -f 
    ${CMAKE_BINARY_DIR}/CMakeCache.txt
    ${CMAKE_BINARY_DIR}/cmake_install.cmake
    ${CMAKE_BINARY_DIR}/Makefile
    ${CMAKE_BINARY_DIR}/chip8
    ${CMAKE_BINARY_DIR}/chip8_config
    ${CMAKE_BINARY_DIR}/libchip8_core.a
    ${CMAKE_BINARY_DIR}/libchip8_platform_sdl.a
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/build
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/null
  COMMENT "Cleaning all generated files and build artifacts..."
)

