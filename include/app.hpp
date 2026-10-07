#pragma once

#include <C2Core/time_core.hpp>
#include <music_player_ui.hpp>
#include <player.hpp>
#include <render.hpp>
#include <webgpu_context.hpp>
#include <window.hpp>

namespace c2 {

struct AppContext {
    gpu::GPUContext gpu{};
    platform::WindowData window{};
    render::RendererState renderer{};

    audio::PlayerState player{};
    audio::PlayerViewData viewData{};
    ui::MusicPlayerUIState ui{};
    std::vector<audio::Command> commandQueue;

    C2Core::Time::Context* timeCtx = nullptr;

    bool sdlInitialized = false;
    bool imguiContextInitialized = false;
    bool imguiSDLInitialized = false;
    bool imguiWGPUInitialized = false;
};

bool initApp(AppContext& app);
int runApp(AppContext& app);
void shutdownApp(AppContext& app);

}  // namespace c2