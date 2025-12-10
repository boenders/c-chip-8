cmake_minimum_required(VERSION 3.23)
project(
    c-chip-8
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Options you can toggle
option(ENABLE_WARNINGS "Enable strict compiler warnings" OFF)
option(ENABLE_SANITIZERS "Enable Address/UB sanitizers in Debug" ON)
option(ENABLE_TESTING "Enable tests with CTest" ON)

# Common compile options
if(ENABLE_WARNINGS)
  if(MSVC)
    add_compile_options(/W4 /permissive-)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic)
  endif()
endif()

# Sanitizers (Debug only, non-MSVC)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()



add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS src/*.c)
add_executable(c-chip-8 ${APP_SOURCES})
target_include_directories(c-chip-8 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(c-chip-8 PRIVATE BL_VERSION="${PROJECT_VERSION}")
target_link_libraries(c-chip-8 PRIVATE SDL3::SDL3)

# if (ENABLE_TESTING)
#     include(CTest)
#     add_subdirectory(tests)
# endif()
