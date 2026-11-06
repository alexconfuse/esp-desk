cmake_minimum_required(VERSION 3.16)
project(desk_buddy_sim C)

# Separate CMake project from the ESP-IDF firmware (root CMakeLists.txt).
# Building here never invokes idf.py and does not touch firmware artifacts.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT DEFINED ENV{CC})
    message(STATUS "Hint: host sim is validated with gcc/g++ (CC=gcc CXX=g++)")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

find_package(CURL)
set(DESK_BUDDY_SIM_HAS_CURL 0)
if(CURL_FOUND)
    set(DESK_BUDDY_SIM_HAS_CURL 1)
    message(STATUS "libcurl found — optional live OpenAI-compatible chat enabled")
else()
    message(STATUS "libcurl not found — mock-only LLM (live path disabled at compile time)")
endif()

include(FetchContent)
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.2.2
    GIT_SHALLOW    TRUE
)
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lvgl)

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
    chat/sim_config.c
    chat/sim_llm.c
    chat/sim_session.c
    ui/sim_type_ui.c
    ui/sim_settings_ui.c
)

target_include_directories(desk_buddy_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/host
    ${CMAKE_CURRENT_SOURCE_DIR}/chat
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${FACE_DIR}
    ${UI_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(desk_buddy_sim PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    DESK_BUDDY_HOST_SIM=1
)

if(DESK_BUDDY_SIM_HAS_CURL)
    target_compile_definitions(desk_buddy_sim PRIVATE DESK_BUDDY_SIM_HAS_CURL=1)
    target_include_directories(desk_buddy_sim PRIVATE ${CURL_INCLUDE_DIRS})
    target_link_libraries(desk_buddy_sim PRIVATE ${CURL_LIBRARIES})
endif()

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
