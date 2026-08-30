#include <SDL3/SDL.h>
#include <sdl3webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

#include <C2Core/c2_log.hpp>
#include <webgpu_context.hpp>
#include <webgpu_utils.hpp>
#include <window.hpp>

extern "C" {
#include <miniaudio.h>
}

#include <cstdlib>
#include <iostream>

const char* shader = R"(
        @vertex fn vs(@builtin(vertex_index) VertexIndex : u32)
                            -> @builtin(position) vec4f {
            var pos = array(
                vec2f( 0.0,  0.5),
                vec2f(-0.5, -0.5),
                vec2f( 0.5, -0.5)
            );
            return vec4f(pos[VertexIndex], 0, 1);
        }

        @fragment fn fs() -> @location(0) vec4f {
            return vec4f(1, 0, 0, 1);
        }
    )";

bool pollEvent(int& running, c2::WindowData& data) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                running = 0;
            }
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                c2::syncFromWindow(data);
            }
        }
    }

    return true;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        C2Core::Log::error("SDL init error: %s", SDL_GetError());
    };

    c2::GPUContext context;
    c2::getGPUContext(context);
    c2::WindowData windowData = c2::createWindow(context);

    wgpu::ShaderModule module =
        c2::utils::CreateShaderModule(context.device, shader);

    wgpu::RenderPipelineDescriptor renderPipelineDescriptor = {};
    renderPipelineDescriptor.vertex.module = module;
    renderPipelineDescriptor.vertex.bufferCount = 0;
    renderPipelineDescriptor.vertex.buffers = nullptr;

    renderPipelineDescriptor.primitive.topology =
        wgpu::PrimitiveTopology::TriangleList;
    renderPipelineDescriptor.primitive.stripIndexFormat =
        wgpu::IndexFormat::Undefined;
    renderPipelineDescriptor.primitive.frontFace = wgpu::FrontFace::CCW;
    renderPipelineDescriptor.primitive.cullMode = wgpu::CullMode::None;

    wgpu::StencilFaceState stencilFace;
    stencilFace.compare = wgpu::CompareFunction::Always;
    stencilFace.failOp = wgpu::StencilOperation::Keep;
    stencilFace.depthFailOp = wgpu::StencilOperation::Keep;
    stencilFace.passOp = wgpu::StencilOperation::Keep;

    /*
    wgpu::DepthStencilState& cDepthStencil =
        renderPipelineDescriptor.depthStencil;
    cDepthStencil.format = wgpu::TextureFormat::Depth24PlusStencil8;
    cDepthStencil.depthWriteEnabled = wgpu::OptionalBool::False;
    cDepthStencil.depthCompare = wgpu::CompareFunction::Always;
    cDepthStencil.stencilBack = stencilFace;
    cDepthStencil.stencilFront = stencilFace;
    cDepthStencil.stencilReadMask = 0xff;
    cDepthStencil.stencilWriteMask = 0xff;
    cDepthStencil.depthBias = 0;
    cDepthStencil.depthBiasSlopeScale = 0.0;
    cDepthStencil.depthBiasClamp = 0.0;
    */

    renderPipelineDescriptor.depthStencil = nullptr;
    renderPipelineDescriptor.layout = nullptr;

    wgpu::MultisampleState& multisample = renderPipelineDescriptor.multisample;
    multisample.count = 1;
    multisample.mask = 0xFFFFFFFF;
    multisample.alphaToCoverageEnabled = false;

    wgpu::FragmentState fragmentState = {};
    fragmentState.module = module;
    fragmentState.targetCount = 1;

    wgpu::ColorTargetState colorTarget = {};
    colorTarget.format = windowData.currentConfig.format;
    fragmentState.targets = &colorTarget;

    renderPipelineDescriptor.fragment = &fragmentState;

    wgpu::BlendComponent blendComponent;
    blendComponent.srcFactor = wgpu::BlendFactor::One;
    blendComponent.dstFactor = wgpu::BlendFactor::Zero;
    blendComponent.operation = wgpu::BlendOperation::Add;

    wgpu::BlendState blendState = {};
    blendState.alpha = blendComponent;
    blendState.color = blendComponent;

    wgpu::RenderPipeline renderPipeline =
        context.device.CreateRenderPipeline(&renderPipelineDescriptor);

    int running = 1;
    while (running) {
        bool success = pollEvent(running, windowData);
        context.instance.ProcessEvents();

        if (!c2::isSameConfig(windowData.currentConfig,
                              windowData.targetConfig)) {
            C2Core::Log::info("Window Resized. New Size: %dx%d",
                              windowData.targetConfig.width,
                              windowData.targetConfig.height);
            windowData.surface.Configure(&windowData.targetConfig);
            windowData.currentConfig = windowData.targetConfig;
        }

        wgpu::SurfaceTexture surfaceTexture = {};
        windowData.surface.GetCurrentTexture(&surfaceTexture);
        wgpu::TextureView view = surfaceTexture.texture.CreateView();

        wgpu::RenderPassDescriptor renderPassDescriptor = {};
        wgpu::RenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = view;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = wgpu::Color{0.0, 0.0, 0.0, 1.0};

        renderPassDescriptor.colorAttachmentCount = 1;
        renderPassDescriptor.colorAttachments = &colorAttachment;

        wgpu::CommandEncoder encoder = context.device.CreateCommandEncoder();

        wgpu::RenderPassEncoder pass =
            encoder.BeginRenderPass(&renderPassDescriptor);
        pass.SetPipeline(renderPipeline);
        pass.Draw(3);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        context.queue.Submit(1, &commands);

        wgpu::Status presentStatus = windowData.surface.Present();
        if (presentStatus != wgpu::Status::Success) {
            C2Core::Log::error("Present status failed");
            return EXIT_FAILURE;
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

    SDL_DestroyWindow(windowData.window);
    SDL_Quit();

    return EXIT_SUCCESS;
}