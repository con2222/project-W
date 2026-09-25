#pragma once

#include <imgui.h>
#include <webgpu/webgpu_cpp.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <player.hpp>

void DrawMusicPlayerUI(wgpu::TextureView& imageView,
                       const c2::audio::PlayerViewData& viewData,
                       c2::audio::PlayerState& player, ImGuiID dockspaceId = 0);

int GetMaxCharactersThatFit(const char* text, float availableWidth);
