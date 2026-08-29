#include <C2Core/c2_log.hpp>
#include <webgpu_context.hpp>

namespace C2Context {

void getGPUContext(GPUContext& context) {
    static constexpr auto kTimedWaitAny =
        wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor instanceDescriptor = {
        .requiredFeatureCount = 1, .requiredFeatures = &kTimedWaitAny};

    wgpu::Instance instance = wgpu::CreateInstance(&instanceDescriptor);
    if (instance == nullptr) {
        C2Core::Log::error("Instance creation failed!");
    }
    context.instance = instance;

    wgpu::RequestAdapterOptions options = {};
    options.backendType = wgpu::BackendType::D3D12;
    wgpu::Adapter adapter;
    auto adapterCallback = [](wgpu::RequestAdapterStatus status,
                              wgpu::Adapter adapter, wgpu::StringView message,
                              void* userdata) {
        if (status != wgpu::RequestAdapterStatus::Success) {
            C2Core::Log::error("Failed to get an adapter: %s", message);
            return;
        }
        *static_cast<wgpu::Adapter*>(userdata) = adapter;
    };

    auto callbackMode = wgpu::CallbackMode::WaitAnyOnly;
    void* userdata = &adapter;
    instance.WaitAny(instance.RequestAdapter(&options, callbackMode,
                                             adapterCallback, userdata),
                     UINT64_MAX);
    if (adapter == nullptr) {
        C2Core::Log::error("RequestAdapter failed");
        return;
    }
    context.adapter = adapter;

    wgpu::DeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.SetUncapturedErrorCallback([](const wgpu::Device&,
                                                   wgpu::ErrorType errorType,
                                                   wgpu::StringView message) {
        C2Core::Log::error("%d error: %s", errorType, message);
        return;
    });

    wgpu::Device device;
    userdata = &device;
    auto deviceCallback = [](wgpu::RequestDeviceStatus status,
                             wgpu::Device device, wgpu::StringView message,
                             void* userdata) {
        if (status != wgpu::RequestDeviceStatus::Success) {
            C2Core::Log::error("Failed to get a device: %s", message.data);
            return;
        }
        *static_cast<wgpu::Device*>(userdata) = device;
    };

    wgpu::Future deviceFuture = context.adapter.RequestDevice(
        &deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly, deviceCallback,
        userdata);
    context.instance.WaitAny(deviceFuture, UINT64_MAX);

    if (device == nullptr) {
        C2Core::Log::error("RequestDevice failed");
        return;
    }
    context.device = device;

    wgpu::Limits limits = {};

    wgpu::Status success = device.GetLimits(&limits);
    if (success == wgpu::Status::Success) {
        C2Core::Log::info("maxTextireDimension1D: %d",
                          limits.maxTextureDimension1D);
    }
}

}  // namespace C2Context