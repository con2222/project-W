#pragma once

#include <imgui.h>
#include <webgpu/webgpu_cpp.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <player.hpp>

namespace c2 {
namespace render {
struct RendererState;
}
}  // namespace c2

namespace c2::ui {

struct MusicPlayerUIState {
    ImGuiTextFilter search;
    std::vector<int> counters;
    std::vector<float> textTimers;
    std::vector<float> scrollOffsets;
};

void ensureCapacity(MusicPlayerUIState& state, size_t trackCount);

void drawMusicPlayerUI(wgpu::TextureView& imageView,
                       const c2::audio::PlayerViewData& viewData,
                       c2::audio::PlayerState& player,
                       c2::ui::MusicPlayerUIState& uiState,
                       c2::render::RendererState& rendererState,
                       std::vector<c2::audio::Command>& commandQueue,
                       ImGuiID dockspaceId = 0);

int GetMaxCharactersThatFit(const char* text, float availableWidth);
}  // namespace c2::ui
