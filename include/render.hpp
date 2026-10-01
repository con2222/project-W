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
    float padding[3];
};

struct RendererState {
    wgpu::RenderPipeline offscreenPipeline;
    wgpu::Buffer uniformBuffer;
    wgpu::BindGroup bindGroup;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout;
    wgpu::Texture offscreenTexture;
    wgpu::TextureView offscreenTextureView;

    int allocatedWidth;
    int allocatedHeight;

    int activeViewportWidth;
    int activeViewportHeight;

    wgpu::TextureFormat format;
    wgpu::ShaderModule module;
};

void setupRenderpass(RendererState& state, const c2::gpu::GPUContext& context,
                     const c2::platform::WindowData& data);

}  // namespace c2::render
