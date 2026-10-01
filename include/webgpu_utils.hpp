#pragma once

#include <webgpu/webgpu_cpp.h>

namespace c2::utils {

wgpu::ShaderModule createShaderModule(const wgpu::Device& device,
                                      const char* source);

wgpu::ShaderModule createShaderModule(const wgpu::Device& device,
                                      const std::string& source);

template <typename T>
    requires std::integral<T>
T align_up(T value, T alignment) {
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0 &&
           "Alignment must be a power of 2!");
    return (value + (alignment - 1)) & ~(alignment - 1);
}

}  // namespace c2::utils
