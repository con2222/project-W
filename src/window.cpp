#include <backends/imgui_impl_sdl3.h>
#include <sdl3webgpu.h>

#include <C2Core/c2_log.hpp>
#include <cassert>
#include <hardcode.hpp>
#include <webgpu_context.hpp>
#include <window.hpp>

namespace c2::platform {

void syncFromWindow(WindowData& data) {
    int width, height;
    SDL_GetWindowSizeInPixels(data.window, &width, &height);

    data.targetConfig.width = std::max(1u, static_cast<uint32_t>(width));
    data.targetConfig.height = std::max(1u, static_cast<uint32_t>(height));
}

// TODO: Handle errors? Can create new func initWindow(gpu::GPUContext& ctx,
// WindowData& data)

// The checks are performed sequentially:
// 1. SDL_CreateWindow() returned a non-null pointer. In case of an error, you
// can retrieve the description using SDL_GetError(). (SDL Wiki)
// 2. A non-empty Surface was successfully obtained.
// 3. GetCapabilities() completed successfully.
// 4. formatCount, alphaModeCount, and presentModeCount are greater than
// zero—only after this condition is met are you allowed to access the elements
// at index [0].
WindowData createWindow(gpu::GPUContext& ctx) {
    SDL_Window* window =
        SDL_CreateWindow("Main", c2::hard::WINDOW_WIDTH,
                         c2::hard::WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

    wgpu::Surface surface = SDL_GetWGPUSurface(ctx.instance.Get(), window);
    C2Core::Log::info("surface = %p", reinterpret_cast<void*>(surface.Get()));

    wgpu::SurfaceCapabilities caps;
    surface.GetCapabilities(ctx.adapter, &caps);

    wgpu::SurfaceConfiguration config;
    config.device = ctx.device;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.format = caps.formats[0];
    config.alphaMode = caps.alphaModes[0];
    config.presentMode = caps.presentModes[0];
    config.width = 0;
    config.height = 0;

    WindowData data;
    data.window = window;
    data.surface = surface;
    data.currentConfig = config;
    data.targetConfig = config;

    syncFromWindow(data);  // set width and height
    surface.Configure(&data.currentConfig);

    // TODO: assign presentModes, alphaModes, formats

    return data;
}

bool initWindow(WindowData& data, gpu::GPUContext& ctx) {
    SDL_Window* window =
        SDL_CreateWindow("Main", c2::hard::WINDOW_WIDTH,
                         c2::hard::WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        C2Core::Log::error("Can't initialize SDL window");
        return false;
    }
    data.window = window;

    wgpu::Surface surface = SDL_GetWGPUSurface(ctx.instance.Get(), window);
    if (!surface) {
        C2Core::Log::error("Can't get surface from OS");
        return false;
    }
    data.surface = surface;

    C2Core::Log::info("surface = %p", reinterpret_cast<void*>(surface.Get()));

    wgpu::SurfaceCapabilities caps;
    if (surface.GetCapabilities(ctx.adapter, &caps) != wgpu::Status::Success) {
        C2Core::Log::error("Can't get surface capabilities");
        return false;
    }

    if (caps.formatCount == 0 || caps.alphaModeCount == 0 ||
        caps.presentModeCount == 0) {
        C2Core::Log::error("Required surface capabilities are missing");
        return false;
    }

    wgpu::SurfaceConfiguration config;
    config.device = ctx.device;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.format = caps.formats[0];
    config.alphaMode = caps.alphaModes[0];
    config.presentMode = caps.presentModes[0];
    config.width = 0;
    config.height = 0;

    data.currentConfig = config;
    data.targetConfig = config;

    syncFromWindow(data);
    data.surface.Configure(&data.targetConfig);
    data.currentConfig = data.targetConfig;

    return true;
}

bool isSameConfig(const wgpu::SurfaceConfiguration& a,
                  const wgpu::SurfaceConfiguration& b) {
    assert(a.viewFormatCount == 0);
    assert(b.viewFormatCount == 0);

    return a.device.Get() == b.device.Get() && a.format == b.format &&
           a.usage == b.usage && a.alphaMode == b.alphaMode &&
           a.width == b.width && a.height == b.height &&
           a.presentMode == b.presentMode;
}

bool pollEvent(int& running, WindowData& data) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                running = 0;
                break;
            }
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                c2::platform::syncFromWindow(data);
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

}  // namespace c2::platform
