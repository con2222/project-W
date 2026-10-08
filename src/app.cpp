#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>

#include <C2Core/c2_log.hpp>
#include <app.hpp>
#include <imgui_layer.hpp>

namespace c2 {
bool initApp(AppContext& app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        C2Core::Log::error("SDL init error: %s", SDL_GetError());
        return false;
    };
    app.sdlInitialized = true;

    if (!c2::gpu::initGPUContext(app.gpu)) {
        return false;
    }

    if (!platform::initWindow(app.window, app.gpu)) {
        return false;
    }

    if (!initImGui(app)) {
        return false;
    }

    c2::render::initRenderer(app.renderer, app.gpu, app.window);

    app.player.playlist.tracks = c2::audio::scanDirectory(
        "music");  // TODO: Change config from .toml or .json

    if (c2::audio::initAudio(app.player.audio) != MA_SUCCESS) {
        C2Core::Log::error("Failed initialize audio engine");
        return false;
    }

    if (c2::audio::initAudioAnalysisNode(app.player.audio) != MA_SUCCESS) {
        C2Core::Log::error("Can't init autio analysis node");
        return false;
    }

    // TODO: Need other handle error
    if (c2::audio::attachAudioAnalysisNodeToEngine(app.player.audio) !=
        MA_SUCCESS) {
        C2Core::Log::error("Can't attach audio analysis node to engine");
        return false;
    }

    if (app.player.playlist.tracks.empty()) {
        C2Core::Log::warning(
            "Playlist is empty. No tracks found in directory.");
    }

    app.timeCtx = C2Core::Time::create(60, 60);
    double targetFPS = 120;
    C2Core::Time::setTargetFPS(app.timeCtx, targetFPS);

    return true;
}

int runApp(AppContext& app) {
    int running = 1;
    ImGuiIO& io = ImGui::GetIO();

    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    while (running) {
        C2Core::Time::startFrame(app.timeCtx);
        float deltaTime = C2Core::Time::getDeltaTime(app.timeCtx);

        bool success = c2::platform::pollEvent(running, app.window);
        if (!running) {
            break;
        }

        app.gpu.instance.ProcessEvents();

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        const ImGuiID dockspaceID =
            ImGui::DockSpaceOverViewport(0, nullptr, dockspace_flags);

        // --- main loop ---
        c2::audio::updatePlayerViewData(app.player, app.viewData);
        c2::ui::drawMusicPlayerUI(
            app.renderer.offscreenTextureView, app.viewData, app.player, app.ui,
            app.renderer, app.commandQueue, app.gpu.device, dockspaceID);
        c2::audio::updatePlayer(app.player, app.commandQueue);
        c2::render::updateRenderer(app.renderer,
                                   app.player.audio.audioAnalysisNode, app.gpu,
                                   deltaTime);

        ImGui::Render();

        bool renderResult =
            c2::render::renderFrame(app.renderer, app.gpu, app.window);

        if (renderResult) {
            wgpu::Status presentStatus = app.window.surface.Present();
            if (presentStatus != wgpu::Status::Success) {
                C2Core::Log::error("Present status failed");
                return EXIT_FAILURE;
            }
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // std::cout << ImGui::GetIO().Framerate << '\n';

        C2Core::Time::endFrame(app.timeCtx, C2Core::Time::WaitMode::Hybrid);
    }

    return EXIT_SUCCESS;
}

void shutdownApp(AppContext& app) {
    if (app.timeCtx != nullptr) {
        C2Core::Time::destroy(app.timeCtx);
        app.timeCtx = nullptr;
    }

    auto& audioState = app.player.audio;
    c2::audio::uninitSound(audioState);

    if (audioState.analysisNodeInitialized) {
        ma_node_uninit(&audioState.audioAnalysisNode, nullptr);
        audioState.analysisNodeInitialized = false;
    }

    if (audioState.engine != nullptr) {
        ma_engine_uninit(audioState.engine);
        delete audioState.engine;
        audioState.engine = nullptr;
    }

    if (app.imguiWGPUInitialized) {
        ImGui_ImplWGPU_Shutdown();
        app.imguiWGPUInitialized = false;
    }

    if (app.imguiSDLInitialized) {
        ImGui_ImplSDL3_Shutdown();
        app.imguiSDLInitialized = false;
    }

    if (app.imguiContextInitialized) {
        ImGui::DestroyContext();
        app.imguiContextInitialized = false;
    }

    app.renderer = c2::render::RendererState{};
    app.window.surface = nullptr;
    app.window.currentConfig = wgpu::SurfaceConfiguration{};
    app.window.targetConfig = wgpu::SurfaceConfiguration{};

    if (app.window.window != nullptr) {
        SDL_DestroyWindow(app.window.window);
        app.window.window = nullptr;
    }

    app.gpu.queue = nullptr;
    app.gpu.device = nullptr;
    app.gpu.adapter = nullptr;
    app.gpu.instance = nullptr;

    if (app.sdlInitialized) {
        SDL_Quit();
        app.sdlInitialized = false;
    }
}

}  // namespace c2