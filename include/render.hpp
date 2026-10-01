#pragma once

#include <webgpu/webgpu_cpp.h>

#include <cstdint>

namespace c2 {
namespace gpu {
struct GPUContext;
}
namespace platform {
struct WindowData;
}
}  // namespace c2

namespace c2::render {

struct Uniforms {
    float pcmFrames;
    float padding;
    float resolution[2];
};

struct RendererState {
    wgpu::RenderPipeline offscreenPipeline;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::Texture offscreenTexture;
    wgpu::TextureView offscreenTextureView;

    uint32_t allocatedWidth;
    uint32_t allocatedHeight;

    uint32_t activeViewportWidth;
    uint32_t activeViewportHeight;

    wgpu::TextureFormat format;
    wgpu::ShaderModule module;
};

void setupRenderpass(RendererState& state, const c2::gpu::GPUContext& context,
                     const c2::platform::WindowData& data);

bool isSameSize(RendererState& state, uint32_t width, uint32_t height);
void recreateTexture(RendererState& state, const wgpu::Device& device,
                     uint32_t width, uint32_t height);

}  // namespace c2::render
