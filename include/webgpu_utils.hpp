#pragma once

#include <webgpu/webgpu_cpp.h>

namespace c2::utils {

wgpu::ShaderModule createShaderModule(const wgpu::Device& device,
                                      const char* source);

wgpu::ShaderModule createShaderModule(const wgpu::Device& device,
                                      const std::string& source);

}  // namespace c2::utils
