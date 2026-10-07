#pragma once

namespace c2 {
struct AppContext;

namespace gpu {
struct GPUContext;
}
namespace platform {
struct WindowData;
}

bool initImGui(AppContext& app);

}  // namespace c2

// void initImGui(c2::gpu::GPUContext& ctx, c2::platform::WindowData& data);
