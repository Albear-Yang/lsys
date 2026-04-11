cmake_minimum_required(VERSION 3.16)
project(LSystemStudio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── find system libraries ─────────────────────────────────────────────
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# ── Dear ImGui ────────────────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ── your sources (FIXED) ──────────────────────────────────────────────
set(LSYS_SOURCES
    src/lsysparse.cc
    src/lsystokenize.cc
    src/lsystype.cc
    src/lsysgen.cc
)

add_executable(lsys_studio
    src/main.cc
    ${LSYS_SOURCES}
    ${IMGUI_SOURCES}
)

# ── include directories (FIXED) ───────────────────────────────────────
target_include_directories(lsys_studio PRIVATE
    ${CMAKE_SOURCE_DIR}/include   # ← THIS is important
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)

# ── link ──────────────────────────────────────────────────────────────
target_link_libraries(lsys_studio PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    dl
)
