#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>
#include <sdl3webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

#include <C2Core/c2_log.hpp>
#include <audio.hpp>
#include <hardcode.hpp>
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

    c2::audio::AudioState audioEngine;
    if (c2::audio::initAudio(audioEngine) != MA_SUCCESS) {
        C2Core::Log::error("Failed initialize audio engine");
        delete audioEngine.engine;
        return EXIT_FAILURE;
    }

    if (c2::audio::initSoundFromFile(audioEngine, "1.mp3") != MA_SUCCESS) {
        C2Core::Log::error("Can't init sound");
        ma_engine_uninit(audioEngine.engine);
        delete audioEngine.engine;
        return EXIT_FAILURE;
    }

    int running = 1;
    float soundVolume = 1.0f;
    c2::audio::soundSetVolume(audioEngine, soundVolume);
    ma_uint64 soundLength;
    ma_uint64 currentSoundLength;
    c2::audio::AudioFormatInfo audioData;

    if (c2::audio::getLengthPCMFrames(audioEngine, soundLength) != MA_SUCCESS) {
        C2Core::Log::error("Can't get pcmf length");
        c2::audio::shutdownAudio(audioEngine);
        return EXIT_FAILURE;
    }
    if (c2::audio::getCursorPCMFrames(audioEngine, currentSoundLength) !=
        MA_SUCCESS) {
        C2Core::Log::error("Can't get cursor pcmf");
        c2::audio::shutdownAudio(audioEngine);
        return EXIT_FAILURE;
    }
    if (c2::audio::getSoundData(audioEngine, audioData) != MA_SUCCESS) {
        C2Core::Log::error("Can't get sound data");
        c2::audio::shutdownAudio(audioEngine);
        return EXIT_FAILURE;
    }

    float progress_value;
    uint32_t currentLengthSeconds = currentSoundLength / audioData.pSampleRate;
    uint32_t soundLengthSeconds = soundLength / audioData.pSampleRate;
    char overlay[32];

    while (running) {
        bool success = pollEvent(running, windowData);
        context.instance.ProcessEvents();

        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpaceOverViewport(0, nullptr, dockspace_flags);

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        ImGui::Begin("Player");

        bool isPlaying = ma_sound_is_playing(&audioEngine.sound);
        bool atEnd = ma_sound_at_end(&audioEngine.sound);

        if (ImGui::Button(isPlaying ? "Pause" : atEnd ? "Replay" : "Play")) {
            if (isPlaying) {
                if (c2::audio::pauseSound(audioEngine) != MA_SUCCESS) {
                    C2Core::Log::error("Can't stop sound");
                    break;
                }
            } else {
                if (c2::audio::playSound(audioEngine) != MA_SUCCESS) {
                    C2Core::Log::error("Can't play sound");
                    break;
                }
            }
        }

        uint64_t minTime = 0;
        uint64_t maxTime = soundLength;

        c2::audio::getCursorPCMFrames(audioEngine, currentSoundLength);
        if (ImGui::SliderScalar("Sound time", ImGuiDataType_U64,
                                &currentSoundLength, &minTime, &maxTime, "%llu",
                                ImGuiSliderFlags_AlwaysClamp)) {
            c2::audio::soundSeekToPCMFrame(audioEngine, currentSoundLength);
        }

        progress_value = float(currentSoundLength) / soundLength;
        currentLengthSeconds = currentSoundLength / audioData.pSampleRate;
        sprintf_s(overlay, "%02u:%02u / %02u:%02u",
                  static_cast<unsigned>(currentLengthSeconds / 60),
                  static_cast<unsigned>(currentLengthSeconds % 60),
                  static_cast<unsigned>(soundLengthSeconds / 60),
                  static_cast<unsigned>(soundLengthSeconds % 60));

        ImGui::ProgressBar(progress_value, ImVec2(0.f, 0.f), overlay);

        ImGui::Spacing();
        if (ImGui::SliderFloat("Sound Volume", &soundVolume, 0.f, 1.f, "%.2f",
                               ImGuiSliderFlags_AlwaysClamp)) {
            c2::audio::soundSetVolume(audioEngine, soundVolume);
        }

        ImGui::End();

        ImGui::Render();

        wgpu::SurfaceTexture surfaceTexture = {};
        windowData.surface.GetCurrentTexture(&surfaceTexture);
        wgpu::TextureView view = surfaceTexture.texture.CreateView();

        wgpu::RenderPassDescriptor renderPassDescriptor = {};
        wgpu::RenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = view;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = wgpu::Color{0.0, 0.0, 1.0, 1.0};

        renderPassDescriptor.colorAttachmentCount = 1;
        renderPassDescriptor.colorAttachments = &colorAttachment;

        wgpu::CommandEncoder encoder = context.device.CreateCommandEncoder();

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
    }

    wgpu::DawnAdapterPropertiesPowerPreference power_props{};

    wgpu::AdapterInfo info{};
    info.nextInChain = &power_props;

    context.adapter.GetInfo(&info);
    std::cout << "VendorID: " << std::hex << info.vendorID << std::dec << "\n";
    std::cout << "Vendor: " << info.vendor << "\n";
    std::cout << "Architecture: " << info.architecture << "\n";
    std::cout << "DeviceID: " << std::hex << info.deviceID << std::dec << "\n";
    std::cout << "Name: " << info.device << "\n";
    std::cout << "Driver description: " << info.description << "\n";
    std::cout << power_props.powerPreference << '\n';

    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    c2::audio::shutdownAudio(audioEngine);

    SDL_DestroyWindow(windowData.window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
