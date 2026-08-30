#include <sdl3webgpu.h>

#include <C2Core/c2_log.hpp>
#include <cassert>
#include <webgpu_context.hpp>
#include <window.hpp>

namespace c2 {

void syncFromWindow(WindowData& data) {
    int width, height;
    SDL_GetWindowSizeInPixels(data.window, &width, &height);

    data.targetConfig.width = std::max(1u, static_cast<uint32_t>(width));
    data.targetConfig.height = std::max(1u, static_cast<uint32_t>(height));
}

WindowData createWindow(GPUContext ctx) {
    SDL_Window* window =
        SDL_CreateWindow("Main", 1920, 1080, SDL_WINDOW_RESIZABLE);

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

    // TODO: assign presentModes, alphaModes, formats

    return data;
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

}  // namespace c2