#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>

#include <C2Core/c2_log.hpp>
#include <app.hpp>
#include <imgui_layer.hpp>
#include <webgpu_context.hpp>
#include <window.hpp>

namespace c2 {

bool initImGui(AppContext& app) {
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    if (main_scale <= 0.0f) {
        C2Core::Log::warning("Can't get UI scale: %s", SDL_GetError());
        main_scale = 1.0f;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    app.imguiContextInitialized = true;

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

    if (!ImGui_ImplSDL3_InitForOther(app.window.window)) {
        C2Core::Log::error("ImGui SDL3 backend initialization failed");
        return false;
    }
    app.imguiSDLInitialized = true;

    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = app.gpu.device.Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat =
        static_cast<WGPUTextureFormat>(app.window.currentConfig.format);
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;

    if (!ImGui_ImplWGPU_Init(&init_info)) {
        C2Core::Log::error("ImGui WGPU backend initialization failed");
        return false;
    }

    app.imguiWGPUInitialized = true;
    return true;
}

}  // namespace c2