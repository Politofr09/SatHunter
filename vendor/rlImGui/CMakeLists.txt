cmake_minimum_required(VERSION 3.0.0)

set(IMGUI_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    rlImGui.cpp
)

set(IMGUI_HEADERS
    imconfig.h
    imgui.h
    imgui_impl_raylib.h
    imgui_internal.h
    imgui_tables.cpp
    imgui_widgets.cpp
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    rlImGui.h
    rlImGuiColors.h
)

add_library(rlImGui STATIC ${IMGUI_SOURCES})

target_include_directories(rlImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link Raylib with rlImGui
target_link_libraries(rlImGui PRIVATE raylib)