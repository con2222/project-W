#include <string>
#include <webgpu_utils.hpp>

namespace c2::utils {

wgpu::ShaderModule CreateShaderModule(const wgpu::Device& device,
                                      const char* source) {
    wgpu::ShaderSourceWGSL wgslDesc;
    wgslDesc.code = source;
    wgpu::ShaderModuleDescriptor descriptor;
    descriptor.nextInChain = &wgslDesc;
    return device.CreateShaderModule(&descriptor);
}

wgpu::ShaderModule CreateShaderModule(const wgpu::Device& device,
                                      const std::string& source) {
    return CreateShaderModule(device, source.c_str());
}

}  // namespace c2::utils