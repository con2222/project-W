#include <backends/imgui_impl_wgpu.h>

#include <C2Core/c2_log.hpp>
#include <hardcode.hpp>
#include <render.hpp>
#include <webgpu_context.hpp>
#include <webgpu_utils.hpp>
#include <window.hpp>

namespace c2::render {
void initRenderer(RendererState& state, const c2::gpu::GPUContext& context,
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
    state.activeViewportWidth = data.targetConfig.width;
    state.activeViewportHeight = data.targetConfig.height;

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

void recreateTexture(RendererState& state, const wgpu::Device& device,
                     uint32_t width, uint32_t height) {
    wgpu::TextureDescriptor textureDesc = {};
    textureDesc.dimension = wgpu::TextureDimension::e2D;
    textureDesc.size = {width, height};
    textureDesc.usage = wgpu::TextureUsage::RenderAttachment |
                        wgpu::TextureUsage::TextureBinding;
    textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;

    state.offscreenTexture = device.CreateTexture(&textureDesc);
    state.offscreenTextureView = state.offscreenTexture.CreateView();

    state.allocatedWidth = width;
    state.allocatedHeight = height;
}

bool isSameSize(RendererState& state, uint32_t width, uint32_t height) {
    return state.allocatedHeight == height && state.allocatedWidth == width;
}

void updateRenderer(RendererState& state, const c2::gpu::GPUContext& ctx,
                    float positionSeconds) {
    uint32_t alignViewportWidth =
        c2::utils::align_up<uint32_t>(state.activeViewportWidth, 64);
    uint32_t alignViewportHeight =
        c2::utils::align_up<uint32_t>(state.activeViewportHeight, 64);

    if (!isSameSize(state, alignViewportWidth, alignViewportHeight)) {
        C2Core::Log::info(
            "Resize texture: allocated width %d %d / active viewport %d %d",
            state.allocatedWidth, state.allocatedHeight,
            state.activeViewportWidth, state.activeViewportHeight);
        recreateTexture(state, ctx.device, alignViewportWidth,
                        alignViewportHeight);
    }

    Uniforms un{};
    un.pcmFrames = positionSeconds;
    un.resolution[0] = static_cast<float>(state.activeViewportWidth);
    un.resolution[1] = static_cast<float>(state.activeViewportHeight);

    ctx.queue.WriteBuffer(state.uniformBuffer, 0, &un, sizeof(Uniforms));
}

void renderFrame(const RendererState& state, const c2::gpu::GPUContext& ctx,
                 const c2::platform::WindowData& windowData) {
    wgpu::SurfaceTexture surfaceTexture = {};
    windowData.surface.GetCurrentTexture(&surfaceTexture);
    wgpu::TextureView view = surfaceTexture.texture.CreateView();

    wgpu::CommandEncoder encoder = ctx.device.CreateCommandEncoder();

    // --- 1. Offscreen rendering ---
    wgpu::RenderPassColorAttachment sceneColorAttachment = {};
    sceneColorAttachment.view = state.offscreenTextureView;
    sceneColorAttachment.loadOp = wgpu::LoadOp::Clear;
    sceneColorAttachment.storeOp = wgpu::StoreOp::Store;
    sceneColorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};

    wgpu::RenderPassDescriptor scenePassDesc = {};
    scenePassDesc.colorAttachmentCount = 1;
    scenePassDesc.colorAttachments = &sceneColorAttachment;

    wgpu::RenderPassEncoder scenePass = encoder.BeginRenderPass(&scenePassDesc);
    scenePass.SetViewport(0, 0, state.activeViewportWidth,
                          state.activeViewportHeight, 0.0f, 1.0f);
    scenePass.SetScissorRect(0, 0, state.activeViewportWidth,
                             state.activeViewportHeight);
    scenePass.SetPipeline(state.offscreenPipeline);
    scenePass.SetBindGroup(0, state.bindGroup);
    scenePass.Draw(3);
    scenePass.End();

    // --- 2. Window rendering (ImGui) ---
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = view;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = wgpu::Color{1.0, 1.0, 1.0, 1.0};

    wgpu::RenderPassDescriptor renderPassDescriptor = {};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;

    wgpu::RenderPassEncoder pass =
        encoder.BeginRenderPass(&renderPassDescriptor);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    ctx.queue.Submit(1, &commands);
}

}  // namespace c2::render
