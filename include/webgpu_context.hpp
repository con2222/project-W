#pragma once

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

namespace C2Context {

struct GPUContext {
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;
};

void getGPUContext(GPUContext& context);

}  // namespace C2Context