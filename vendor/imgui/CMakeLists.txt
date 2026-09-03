add_library(imgui STATIC
    backends/imgui_impl_wgpu.h
    backends/imgui_impl_wgpu.cpp
    backends/imgui_impl_sdl3.h
    backends/imgui_impl_sdl3.cpp

    misc/cpp/imgui_stdlib.cpp
    misc/cpp/imgui_stdlib.h

    imconfig.h
    imgui.h
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_tables.cpp
    imgui_widgets.cpp
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
)

target_include_directories(imgui PUBLIC .)
target_link_libraries(imgui PUBLIC SDL3::SDL3 webgpu)