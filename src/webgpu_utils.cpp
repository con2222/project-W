#include <cassert>
#include <concepts>
#include <string>
#include <webgpu_utils.hpp>

namespace c2::utils {

wgpu::ShaderModule createShaderModule(const wgpu::Device& device,
                                      const char* source) {
    wgpu::ShaderSourceWGSL wgslDesc;
    wgslDesc.code = source;
    wgpu::ShaderModuleDescriptor descriptor;
    descriptor.nextInChain = &wgslDesc;
    return device.CreateShaderModule(&descriptor);
}

wgpu::ShaderModule createShaderModule(const wgpu::Device& device,
                                      const std::string& source) {
    return createShaderModule(device, source.c_str());
}

}  // namespace c2::utils
