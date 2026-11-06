cmake_minimum_required(VERSION 3.16)
project(desk_buddy_sim C)

# Separate CMake project from the ESP-IDF firmware (root CMakeLists.txt).
# Building here never invokes idf.py and does not touch firmware artifacts.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Prefer GCC when the default is Clang-as-cc without a working host ASM path.
if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT DEFINED ENV{CC})
    message(STATUS "Hint: host sim is validated with gcc/g++ (CC=gcc CXX=g++)")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

include(FetchContent)
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.2.2
    GIT_SHALLOW    TRUE
)
# Point LVGL at this directory's lv_conf.h; skip demos/examples for faster host builds.
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lvgl)

# MCU-only SW blend .S files #include C headers and break the host assembler.
# Prefer pure C SW blend on PC (LV_USE_DRAW_SW_ASM is NONE in lv_conf.h).
get_target_property(_lvgl_srcs lvgl SOURCES)
if(_lvgl_srcs)
    list(FILTER _lvgl_srcs EXCLUDE REGEX ".*\\.S$")
    set_target_properties(lvgl PROPERTIES SOURCES "${_lvgl_srcs}")
endif()

set(FACE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../main/face")
set(UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../main/ui")

add_executable(desk_buddy_sim
    main.c
    ${FACE_DIR}/face_ui.c
    ${UI_DIR}/action_panel.c
)

target_include_directories(desk_buddy_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/host
    ${FACE_DIR}
    ${UI_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(desk_buddy_sim PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    DESK_BUDDY_HOST_SIM=1
)

# Ensure LVGL itself sees lv_conf.h and SDL headers while compiling its SDL driver.
target_include_directories(lvgl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_compile_options(lvgl PRIVATE ${SDL2_CFLAGS_OTHER})

target_link_libraries(desk_buddy_sim PRIVATE
    lvgl
    ${SDL2_LIBRARIES}
    m
)

target_compile_options(desk_buddy_sim PRIVATE
    ${SDL2_CFLAGS_OTHER}
    -Wall
    -Wextra
)

install(TARGETS desk_buddy_sim RUNTIME DESTINATION bin)
