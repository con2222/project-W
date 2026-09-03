#pragma once

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

namespace c2::gpu {

struct GPUContext {
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;
};

GPUContext getGPUContext();

wgpu::Instance initInstance();
wgpu::Adapter createAdapter(const wgpu::Instance& instance);
wgpu::Device createDevice(const wgpu::Instance& instance,
                          const wgpu::Adapter& adapter);
wgpu::Queue createQueue(const wgpu::Device& device);

}  // namespace c2::gpu