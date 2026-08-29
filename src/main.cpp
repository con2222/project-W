#include <SDL3/SDL.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

#include <C2Core/c2_log.hpp>
#include <webgpu_context.hpp>

extern "C" {
#include <miniaudio.h>
}

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    C2Context::GPUContext context;
    C2Context::getGPUContext(context);

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
    return EXIT_SUCCESS;
}