#include <hardcode.hpp>
#include <render.hpp>
#include <webgpu_context.hpp>
#include <webgpu_utils.hpp>
#include <window.hpp>

namespace c2::render {
void setupRenderpass(RendererState& state, const c2::gpu::GPUContext& context,
                     const c2::platform::WindowData& data) {
    wgpu::BufferDescriptor uniformBufferDesc = {};
    uniformBufferDesc.mappedAtCreation = false;
    uniformBufferDesc.size = sizeof(c2::render::Uniforms);
    uniformBufferDesc.usage =
        wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    state.uniformBuffer = context.device.CreateBuffer(&uniformBufferDesc);

    std::vector<wgpu::BindGroupLayoutEntry> BGLayoutEntries(1);

    BGLayoutEntries[0].binding = 0;
    BGLayoutEntries[0].buffer.type = wgpu::BufferBindingType::Uniform;
    BGLayoutEntries[0].buffer.minBindingSize = sizeof(c2::render::Uniforms);
    BGLayoutEntries[0].buffer.hasDynamicOffset = false;
    BGLayoutEntries[0].visibility =
        wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;

    wgpu::BindGroupLayoutDescriptor BGLayoutDesc = {};
    BGLayoutDesc.entries = BGLayoutEntries.data();
    BGLayoutDesc.entryCount = BGLayoutEntries.size();
    state.bindGroupLayout = context.device.CreateBindGroupLayout(&BGLayoutDesc);

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &state.bindGroupLayout;
    state.pipelineLayout =
        context.device.CreatePipelineLayout(&pipelineLayoutDesc);

    std::vector<wgpu::BindGroupEntry> BGEntries(1);
    BGEntries[0].binding = 0;
    BGEntries[0].buffer = state.uniformBuffer;
    BGEntries[0].offset = 0;
    BGEntries[0].size = sizeof(c2::render::Uniforms);

    wgpu::BindGroupDescriptor BGDesc = {};
    BGDesc.entryCount = BGEntries.size();
    BGDesc.entries = BGEntries.data();
    BGDesc.layout = state.bindGroupLayout;
    state.bindGroup = context.device.CreateBindGroup(&BGDesc);

    // ============================================================================
    // Texture settup
    // ============================================================================

    state.module =
        c2::utils::createShaderModule(context.device, c2::hard::shader1);

    wgpu::TextureDescriptor imageTextureDesc = {};
    imageTextureDesc.dimension = wgpu::TextureDimension::e2D;
    imageTextureDesc.size = {data.targetConfig.width, data.targetConfig.height};
    imageTextureDesc.usage = wgpu::TextureUsage::RenderAttachment |
                             wgpu::TextureUsage::TextureBinding;
    imageTextureDesc.format = wgpu::TextureFormat::RGBA8Unorm;

    state.allocatedWidth = data.targetConfig.width;
    state.allocatedHeight = data.targetConfig.height;

    state.offscreenTexture = context.device.CreateTexture(&imageTextureDesc);
    state.offscreenTextureView = state.offscreenTexture.CreateView();

    // --- Render pipeline setup ---

    wgpu::RenderPipelineDescriptor scenePipelineDesc = {};
    scenePipelineDesc.depthStencil = nullptr;
    scenePipelineDesc.layout = state.pipelineLayout;

    scenePipelineDesc.vertex.buffers = nullptr;
    scenePipelineDesc.vertex.bufferCount = 0;
    scenePipelineDesc.vertex.module = state.module;
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
    sceneFragmentState.module = state.module;
    sceneFragmentState.entryPoint = "scene_fs";
    sceneFragmentState.targetCount = 1;
    wgpu::ColorTargetState sceneColorTarget = {};
    sceneColorTarget.format = wgpu::TextureFormat::RGBA8Unorm;
    sceneFragmentState.targets = &sceneColorTarget;
    scenePipelineDesc.fragment = &sceneFragmentState;

    state.offscreenPipeline =
        context.device.CreateRenderPipeline(&scenePipelineDesc);
}
}  // namespace c2::render
