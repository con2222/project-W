#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <sdl3webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

#include <C2Core/c2_log.hpp>
#include <C2Core/time_core.hpp>
#include <app.hpp>
#include <audio.hpp>
#include <command.hpp>
#include <hardcode.hpp>
#include <imgui_layer.hpp>
#include <interfacetest.hpp>
#include <music_player_ui.hpp>
#include <player.hpp>
#include <render.hpp>
#include <webgpu_context.hpp>
#include <webgpu_utils.hpp>
#include <window.hpp>

extern "C" {
#include <miniaudio.h>
}

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    c2::AppContext app;
    int result = 0;
    if (c2::initApp(app)) {
        result = c2::runApp(app);
    }

    c2::shutdownApp(app);
    return result;
}
