#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu_cpp.h>

namespace c2::gpu {
struct GPUContext;
}

namespace c2::platform {

struct WindowData {
    SDL_Window* window = nullptr;

    std::vector<wgpu::PresentMode> presentModes;
    std::vector<wgpu::CompositeAlphaMode> alphaModes;
    std::vector<wgpu::TextureFormat> formats;

    wgpu::Surface surface = nullptr;
    wgpu::SurfaceConfiguration currentConfig;
    wgpu::SurfaceConfiguration targetConfig;
};

WindowData createWindow(c2::gpu::GPUContext ctx);
void syncFromWindow(WindowData& data);
bool isSameConfig(const wgpu::SurfaceConfiguration& a,
                  const wgpu::SurfaceConfiguration& b);
bool pollEvent(int& running, WindowData& data);

}  // namespace c2::platform
