#pragma once

namespace c2 {
namespace gpu {
struct GPUContext;
}
namespace platform {
struct WindowData;
}
}  // namespace c2

void initImGui(c2::gpu::GPUContext& ctx, c2::platform::WindowData& data);
