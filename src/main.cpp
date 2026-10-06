#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <sdl3webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

#include <C2Core/c2_log.hpp>
#include <C2Core/time_core.hpp>
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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        C2Core::Log::error("SDL init error: %s", SDL_GetError());
        return EXIT_FAILURE;
    };
    c2::gpu::GPUContext context = c2::gpu::getGPUContext();
    c2::platform::WindowData windowData = c2::platform::createWindow(context);
    initImGui(context, windowData);

    c2::audio::PlayerViewData viewData{};
    std::vector<c2::audio::Command> commandQueue;
    c2::ui::MusicPlayerUIState uiState;
    c2::render::RendererState rendererState;
    int running = 1;

    ImGuiIO& io = ImGui::GetIO();
    c2::render::initRenderer(rendererState, context, windowData);

    // ============================================================================
    // Audio settings
    // ============================================================================

    c2::audio::PlayerState player{};
    player.volume = 1.0f;

    player.playlist.tracks = c2::audio::scanDirectory(
        "music");  // TODO: Change config from .toml or .json

    if (c2::audio::initAudio(player.audio) != MA_SUCCESS) {
        C2Core::Log::error("Failed initialize audio engine");
        return EXIT_FAILURE;
    }

    if (c2::audio::initAudioAnalysisNode(player.audio) != MA_SUCCESS) {
        C2Core::Log::error("Can't init autio analysis node");
        return EXIT_FAILURE;
    }

    // TODO: Need other handle error
    if (c2::audio::attachAudioAnalysisNodeToEngine(player.audio) !=
        MA_SUCCESS) {
        C2Core::Log::error("Can't attach audio analysis node to engine");
        return EXIT_FAILURE;
    }

    if (!player.playlist.tracks.empty()) {
        if (!c2::audio::selectTrack(player, 0)) {
            C2Core::Log::error("Failed to select initial track");
        }
    } else {
        C2Core::Log::warning(
            "Playlist is empty. No tracks found in directory.");
    }

    C2Core::Time::Context* timeCtx = C2Core::Time::create(60, 60);
    double targetFPS = 120;
    C2Core::Time::setTargetFPS(timeCtx, targetFPS);

    while (running) {
        C2Core::Time::startFrame(timeCtx);
        float deltaTime = C2Core::Time::getDeltaTime(timeCtx);

        bool success = c2::platform::pollEvent(running, windowData);
        context.instance.ProcessEvents();

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_PassthruCentralNode;
        const ImGuiID dockspaceID =
            ImGui::DockSpaceOverViewport(0, nullptr, dockspace_flags);

        c2::audio::updatePlayerViewData(player, viewData);
        c2::render::updateRenderer(
            rendererState, player.audio.audioAnalysisNode, context, deltaTime);

        c2::ui::drawMusicPlayerUI(rendererState.offscreenTextureView, viewData,
                                  player, uiState, rendererState, commandQueue,
                                  dockspaceID);
        c2::audio::updatePlayer(player, commandQueue);

        ImGui::Render();

        c2::render::renderFrame(rendererState, context, windowData);

        wgpu::Status presentStatus = windowData.surface.Present();
        if (presentStatus != wgpu::Status::Success) {
            C2Core::Log::error("Present status failed");
            break;  // TODO: handling error
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        std::cout << ImGui::GetIO().Framerate << '\n';

        C2Core::Time::endFrame(timeCtx, C2Core::Time::WaitMode::Hybrid);
    }

    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    c2::audio::shutdownAudio(player.audio);

    SDL_DestroyWindow(windowData.window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
