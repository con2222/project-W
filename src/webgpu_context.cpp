#include <C2Core/c2_log.hpp>
#include <webgpu_context.hpp>

namespace c2::gpu {

GPUContext getGPUContext() {
    GPUContext context;
    context.instance = initInstance();
    context.adapter = createAdapter(context.instance);
    context.device = createDevice(context.instance, context.adapter);
    context.queue = context.device.GetQueue();

    return context;
}

wgpu::Instance initInstance() {
    // Init instance
    static constexpr auto kTimedWaitAny =
        wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor instanceDescriptor = {
        .requiredFeatureCount = 1, .requiredFeatures = &kTimedWaitAny};

    wgpu::Instance instance = wgpu::CreateInstance(&instanceDescriptor);
    if (instance == nullptr) {
        C2Core::Log::error("Instance creation failed!");
    }
    return instance;
}

wgpu::Adapter createAdapter(const wgpu::Instance& instance) {
    // Request adapter
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
    void* userdata = &adapter;
    wgpu::Future adapterFuture = instance.RequestAdapter(
        &options, wgpu::CallbackMode::WaitAnyOnly, adapterCallback, userdata);
    instance.WaitAny(adapterFuture, UINT64_MAX);
    if (adapter == nullptr) {
        C2Core::Log::error("RequestAdapter failed");
        return nullptr;
    }

    wgpu::DawnAdapterPropertiesPowerPreference power_props{};

    wgpu::AdapterInfo info{};
    info.nextInChain = &power_props;

    adapter.GetInfo(&info);
    C2Core::Log::info("Vendor: %s", info.vendor.data);
    C2Core::Log::info("Architecture: %s", info.architecture.data);
    C2Core::Log::info("DeviceID: %X", info.deviceID);
    C2Core::Log::info("Name: %s", info.device.data);
    C2Core::Log::info("Driver description: %s", info.description.data);
    C2Core::Log::info("Power preference: %s", power_props.powerPreference);
    return adapter;
};

wgpu::Device createDevice(const wgpu::Instance& instance,
                          const wgpu::Adapter& adapter) {
    // Request Device from adapter
    wgpu::DeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.SetUncapturedErrorCallback([](const wgpu::Device&,
                                                   wgpu::ErrorType errorType,
                                                   wgpu::StringView message) {
        C2Core::Log::error("%d error: %s", errorType, message);
        return;
    });  // error in runtime

    wgpu::Device device;
    void* userdata = &device;
    auto deviceCallback = [](wgpu::RequestDeviceStatus status,
                             wgpu::Device device, wgpu::StringView message,
                             void* userdata) {
        if (status != wgpu::RequestDeviceStatus::Success) {
            C2Core::Log::error("Failed to get a device: %s", message.data);
            return;
        }
        *static_cast<wgpu::Device*>(userdata) = device;
    };  // error initialization

    wgpu::Future deviceFuture = adapter.RequestDevice(
        &deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly, deviceCallback,
        userdata);
    instance.WaitAny(deviceFuture, UINT64_MAX);

    if (device == nullptr) {
        C2Core::Log::error("RequestDevice failed");
        return nullptr;
    }
    return device;
}

wgpu::Queue createQueue(const wgpu::Device& device);

}  // namespace c2::gpu