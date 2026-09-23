#include <algorithm>  // for std::clamp
#include <cmath>      // for std::pow

#include "imgui.h"

// Helper function for linear interpolation
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

inline void DrawAnimatedCustomButton(const char* label) {
    // 1. Generate a unique ID for this widget instance based on its label
    ImGuiID id = ImGui::GetID(label);

    // 2. Access ImGui's key-value storage for the current window.
    // This allows us to store the animation progress specific to this widget ID
    // without static variables.
    ImGuiStorage* storage = ImGui::GetStateStorage();

    // Retrieve current animation progress 't' for this ID. Defaults to 0.0f if
    // it doesn't exist.
    float anim_t = storage->GetFloat(id, 0.0f);

    // 3. Define the base size and get the current cursor position on the screen
    ImVec2 base_size(150.0f, 40.0f);
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    // Create an invisible button to handle user interactions (hover, click) in
    // this area
    ImGui::InvisibleButton(label, base_size);
    bool is_hovered = ImGui::IsItemHovered();
    bool is_active =
        ImGui::IsItemActive();  // True while mouse button is held down

    // 4. Update the animation state based on time
    float dt = ImGui::GetIO().DeltaTime;
    float anim_speed = 8.0f;

    if (is_hovered) {
        anim_t += dt * anim_speed;  // Move forward
    } else {
        anim_t -= dt * anim_speed;  // Move backward
    }

    // Ensure the value strictly stays between 0.0 (idle) and 1.0 (fully active)
    anim_t = std::clamp(anim_t, 0.0f, 1.0f);

    // Save the updated state back to the storage for the next frame
    storage->SetFloat(id, anim_t);

    // 5. Apply an Easing function to make the animation feel natural
    // Ease-out cubic: starts fast, smoothly decelerates
    float ease_out = 1.0f - std::pow(1.0f - anim_t, 3.0f);

    // 6. Calculate visual properties using the eased value
    // Animate width: expands by 30 pixels when hovered
    float current_width = Lerp(base_size.x, base_size.x + 30.0f, ease_out);

    // Determine background color based on interaction state
    ImU32 col_bg;
    if (is_active) {
        col_bg = IM_COL32(30, 90, 200, 255);  // Darker blue when clicked
    } else {
        // Interpolate alpha or channels to fade color. Here we just swap
        // colors, but the widening effect carries the animation.
        col_bg = is_hovered ? IM_COL32(40, 120, 250, 255)
                            : IM_COL32(60, 60, 60, 255);
    }

    // 7. Rendering with ImDrawList
    // Get the draw list for the current window to draw custom shapes
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 p_min = cursor_pos;
    ImVec2 p_max =
        ImVec2(cursor_pos.x + current_width, cursor_pos.y + base_size.y);

    // Draw the rounded rectangle background
    draw_list->AddRectFilled(p_min, p_max, col_bg, 8.0f);

    // Draw the label text perfectly centered within the dynamic width
    ImU32 text_col = IM_COL32(255, 255, 255, 255);
    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(p_min.x + (current_width - text_size.x) * 0.5f,
                             p_min.y + (base_size.y - text_size.y) * 0.5f);

    draw_list->AddText(text_pos, text_col, label);
}
