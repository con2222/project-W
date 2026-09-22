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
#include <hardcode.hpp>
#include <music_player_ui.hpp>
#include <webgpu_context.hpp>
#include <webgpu_utils.hpp>
#include <window.hpp>

extern "C" {
#include <miniaudio.h>
}

#include <cstdlib>
#include <iostream>

void initImGui(c2::gpu::GPUContext& ctx, c2::WindowData data);

bool pollEvent(int& running, c2::WindowData& data) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                running = 0;
                break;
            }
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                c2::syncFromWindow(data);
                C2Core::Log::info("Window Resized. New Size: %dx%d",
                                  data.targetConfig.width,
                                  data.targetConfig.height);
                data.surface.Configure(&data.targetConfig);
                data.currentConfig = data.targetConfig;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) running = false;
        }
    }

    return true;
}

bool setup(c2::gpu::GPUContext& ctx, c2::WindowData data) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        C2Core::Log::error("SDL init error: %s", SDL_GetError());
        return false;
    };

    initImGui(ctx, data);

    return true;
}

void initImGui(c2::gpu::GPUContext& ctx, c2::WindowData data) {
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(
        main_scale);  // Bake a fixed style scale. (until we have a solution for
                      // dynamic style scaling, changing this requires resetting
                      // Style + calling this again)
    style.FontScaleDpi =
        main_scale;  // Set initial font scale. (in docking branch: using
                     // io.ConfigDpiScaleFonts=true automatically overrides this
                     // for every window depending on the current monitor)

    ImGui_ImplSDL3_InitForOther(data.window);

    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = ctx.device.Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat =
        static_cast<WGPUTextureFormat>(data.currentConfig.format);
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);
}

wgpu::ShaderModule createShaderModule(const wgpu::Device& device,
                                      const char* source) {
    wgpu::ShaderSourceWGSL wgslDesc;
    wgslDesc.code = source;
    wgpu::ShaderModuleDescriptor descriptor;
    descriptor.nextInChain = &wgslDesc;
    return device.CreateShaderModule(&descriptor);
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        C2Core::Log::error("SDL init error: %s", SDL_GetError());
        return EXIT_FAILURE;
    };
    c2::gpu::GPUContext context = c2::gpu::getGPUContext();
    c2::WindowData windowData = c2::createWindow(context);
    initImGui(context, windowData);

    ImGuiIO& io = ImGui::GetIO();

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // ============================================================================
    // Texture settup
    // ============================================================================

    wgpu::ShaderModule shaderModule =
        createShaderModule(context.device, c2::hard::shader);

    wgpu::TextureDescriptor imageTextureDesc = {};
    imageTextureDesc.dimension = wgpu::TextureDimension::e2D;
    imageTextureDesc.size = {windowData.targetConfig.width,
                             windowData.targetConfig.height};
    imageTextureDesc.usage = wgpu::TextureUsage::RenderAttachment |
                             wgpu::TextureUsage::TextureBinding;
    imageTextureDesc.format = wgpu::TextureFormat::RGBA8Unorm;

    wgpu::Texture imageTexture =
        context.device.CreateTexture(&imageTextureDesc);
    wgpu::TextureView imageView = imageTexture.CreateView();

    // --- Render pipeline setup ---

    wgpu::RenderPipelineDescriptor scenePipelineDesc = {};
    scenePipelineDesc.depthStencil = nullptr;

    scenePipelineDesc.vertex.buffers = nullptr;
    scenePipelineDesc.vertex.bufferCount = 0;
    scenePipelineDesc.vertex.module = shaderModule;
    scenePipelineDesc.vertex.entryPoint = "scene_vs";

    scenePipelineDesc.primitive.topology =
        wgpu::PrimitiveTopology::TriangleList;
    scenePipelineDesc.primitive.cullMode = wgpu::CullMode::None;
    scenePipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;

    scenePipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
    scenePipelineDesc.depthStencil = nullptr;
    scenePipelineDesc.multisample.count = 1;
    scenePipelineDesc.multisample.mask = 0xFFFFFFFF;
    scenePipelineDesc.multisample.alphaToCoverageEnabled = false;

    wgpu::FragmentState sceneFragmentState = {};
    sceneFragmentState.module = shaderModule;
    sceneFragmentState.entryPoint = "scene_fs";
    sceneFragmentState.targetCount = 1;
    wgpu::ColorTargetState sceneColorTarget = {};
    sceneColorTarget.format = wgpu::TextureFormat::RGBA8Unorm;
    sceneFragmentState.targets = &sceneColorTarget;
    scenePipelineDesc.fragment = &sceneFragmentState;

    wgpu::RenderPipeline scenePipeline =
        context.device.CreateRenderPipeline(&scenePipelineDesc);

    // ============================================================================
    // Audio settings
    // ============================================================================

    c2::audio::PlayerState player{};
    player.volume = 1.0f;
    // Add your actual file paths here. Duration is filled after loading a
    // track.

    player.playlist.tracks = c2::audio::scanDirectory("music");

    C2Core::Log::info("%d", player.playlist.tracks.size());

    if (c2::audio::initAudio(player.audio) != MA_SUCCESS) {
        C2Core::Log::error("Failed initialize audio engine");
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
    double targetFPS = 60;

    int running = 1;
    c2::audio::PlayerViewData viewData{};

    while (running) {
        C2Core::Time::startFrame(timeCtx);
        C2Core::Time::setTargetFPS(timeCtx, targetFPS);
        bool success = pollEvent(running, windowData);
        context.instance.ProcessEvents();

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_PassthruCentralNode;
        const ImGuiID dockspaceID =
            ImGui::DockSpaceOverViewport(0, nullptr, dockspace_flags);

        if (show_demo_window) {
            // ImGui::ShowDemoWindow(&show_demo_window);
        }

        c2::audio::updatePlayerViewData(player, viewData);

        wgpu::SurfaceTexture surfaceTexture = {};
        windowData.surface.GetCurrentTexture(&surfaceTexture);
        wgpu::TextureView view = surfaceTexture.texture.CreateView();

        // --- For texture rendering ---
        wgpu::RenderPassDescriptor scenePassDesc = {};
        wgpu::RenderPassColorAttachment sceneColorAttachment = {};
        sceneColorAttachment.view = imageView;
        sceneColorAttachment.loadOp = wgpu::LoadOp::Clear;
        sceneColorAttachment.storeOp = wgpu::StoreOp::Store;
        sceneColorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};
        scenePassDesc.colorAttachmentCount = 1;
        scenePassDesc.colorAttachments = &sceneColorAttachment;

        // --- For window rendering ---
        wgpu::RenderPassDescriptor renderPassDescriptor = {};
        wgpu::RenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = view;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = wgpu::Color{0.0, 0.0, 1.0, 1.0};
        renderPassDescriptor.colorAttachmentCount = 1;
        renderPassDescriptor.colorAttachments = &colorAttachment;

        wgpu::CommandEncoder encoder = context.device.CreateCommandEncoder();

        wgpu::RenderPassEncoder scenePass =
            encoder.BeginRenderPass(&scenePassDesc);

        scenePass.SetPipeline(scenePipeline);
        scenePass.Draw(3);
        scenePass.End();

        C2Core::Log::info("%f", ImGui::GetIO().Framerate);

        DrawMusicPlayerUI(imageView, viewData, player, dockspaceID);
        ImGui::Render();

        wgpu::RenderPassEncoder pass =
            encoder.BeginRenderPass(&renderPassDescriptor);

        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        context.queue.Submit(1, &commands);

        wgpu::Status presentStatus = windowData.surface.Present();
        if (presentStatus != wgpu::Status::Success) {
            C2Core::Log::error("Present status failed");
            break;  // TODO: handling error
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        C2Core::Time::endFrame(timeCtx, C2Core::Time::WaitMode::Spin);
    }

    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    c2::audio::shutdownAudio(player.audio);

    SDL_DestroyWindow(windowData.window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
