#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>  // for std::clamp
#include <cmath>      // for std::pow
#include <string>

// Helper function for linear interpolation
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

inline void firstTestWidget() {
    /*
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // ImVec2 windowPos = ImGui::GetWindowPos();

    ImGui::Dummy(ImVec2(50.f, 50.f));

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddCircle(ImVec2(cursorPos.x + 25.f, cursorPos.y + 25.f), 25.f,
                        ImColor(255, 100, 100, 255));

    float time = (float)ImGui::GetTime();
    float y_offset = sinf(time * 5.0f) * 5.0f;

    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(pos.x, pos.y + y_offset));

    std::string test = "1234567";
    std::string temp = {};
    temp.resize(5);

    static size_t strLen = test.length();
    static int counter = 0;
    static float textTimer = 0.f;

    const float delay = 0.5f;

    textTimer += ImGui::GetIO().DeltaTime;

    if (textTimer >= delay) {
        counter++;
        counter = counter % strLen;
        textTimer = 0.f;
    }

    for (int i = 0; i < 5; i++) {
        temp[i] = test[(counter + i) % strLen];
    }

    if (ImGui::Selectable(test.c_str())) {
    }
    ImGui::Text("%.*s", 4, temp.c_str());

    */

    /*
    ImGui::Begin("Test");
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 currentPositionInWindows = ImGui::GetCursorScreenPos();

    ImGui::Dummy(ImVec2(100.f, 100.f));

    drawList->AddRectFilled(
        ImVec2(currentPositionInWindows.x, currentPositionInWindows.y),
        ImVec2(currentPositionInWindows.x + 100.f,
               currentPositionInWindows.y + 100.f),
        ImColor(255, 0, 0, 255));

    ImGui::End();
    */
}

inline void myButton(const char* label, ImVec2 size, ImColor buttonColor,
                     ImColor activeColor, ImColor hoveredColor, float rounded) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(label, ImVec2(pos.x + size.x, pos.y + size.y));

    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            buttonColor, rounded);

    if (ImGui::IsItemHovered()) {
        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                                hoveredColor, rounded);
    }

    if (ImGui::IsItemActive()) {
        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                                activeColor, rounded);
    }
    ImGui::Dummy(size);
}

inline void shiftSelectableButton(const char* label, const char* text) {
    ImGuiID id = ImGui::GetID(label);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImGuiStorage* storage = ImGui::GetStateStorage();

    float deltaTIme = ImGui::GetIO().DeltaTime;

    float animationSpeed = 5.f;

    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("adsada", ImVec2(200.f, 100.f));

    if (ImGui::IsItemHovered()) {
        drawList->AddRectFilled(pos, ImVec2(pos.x + 200.f, pos.y + 100.f),
                                ImColor(125, 100, 25, 255), 8.f);
    } else {
        drawList->AddRectFilled(pos, ImVec2(pos.x + 200.f, pos.y + 100.f),
                                ImColor(125, 100, 25, 0), 8.f);
    }

    if (ImGui::IsItemActive()) {
        drawList->AddRectFilled(pos, ImVec2(pos.x + 200.f, pos.y + 100.f),
                                ImColor(140, 235, 25, 255), 8.f);

        drawList->AddText(ImVec2(pos.x + 75.f, pos.y + 50.f),
                          ImColor(140, 0, 0, 255), text);
    }

    ImGui::Dummy(ImVec2(200, 100));
}

inline void shiftText(const char* text) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 pos = ImGui::GetCursorPos();

    drawList->AddRectFilled(pos, ImVec2(pos.x + 200.f, pos.y + 100.f),
                            ImColor(140, 235, 25, 255), 8.f);

    drawList->AddText(pos, ImColor(40, 30, 20, 255), text);
}

inline void HoverableText(const char* text) {
    // 1. Вычисляем размер будущего текста
    ImVec2 textSize = ImGui::CalcTextSize(text);

    // 2. Запоминаем текущую позицию курсора на экране
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // 3. Создаем невидимую кнопку такого же размера
    // "##" скрывает ID кнопки, чтобы избежать конфликтов имен
    ImGui::InvisibleButton("##hidden_btn", textSize);

    // 4. Проверяем, наведена ли мышь на эту невидимую кнопку
    bool isHovered = ImGui::IsItemHovered();

    // 5. Возвращаем курсор на исходную позицию, чтобы нарисовать текст ПОВЕРХ
    // кнопки
    ImGui::SetCursorScreenPos(cursorPos);

    // 6. Рисуем текст в зависимости от состояния
    if (isHovered) {
        // Текст "горит" (например, желтым цветом: RGBA)
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", text);
    } else {
        // Обычный цвет текста (по умолчанию)
        ImGui::Text("%s", text);
    }
}

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

inline void RenderAnimatedMenu() {
    // Static variables to keep state between frames
    static bool is_window_open = false;
    static float anim_progress = 0.0f;

    // Toggle button for testing
    if (ImGui::Button("Toggle Animated Window")) {
        is_window_open = !is_window_open;
    }

    // 1. Update animation progress based on DeltaTime
    float delta_time = ImGui::GetIO().DeltaTime;
    float animation_speed = 3.5f;  // Multiplier for how fast it opens/closes

    if (is_window_open) {
        anim_progress += delta_time * animation_speed;
        if (anim_progress > 1.0f) anim_progress = 1.0f;
    } else {
        anim_progress -= delta_time * animation_speed;
        if (anim_progress < 0.0f) anim_progress = 0.0f;
    }

    // 2. Render window only if it's partially or fully visible
    if (anim_progress > 0.0f) {
        // Simple Ease-Out Cubic function for smoother animation
        // float ease_out = 1.0f - std::pow(1.0f - anim_progress, 3.0f);

        // Calculate sliding position (e.g., sliding down from the top)
        float start_y = -200.0f;  // Hidden above the screen
        float target_y = 50.0f;   // Final resting position
        float current_y = start_y + (target_y - start_y) * anim_progress;

        // Apply animated position
        ImGui::SetNextWindowPos(ImVec2(100.0f, current_y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300.0f, 150.0f), ImGuiCond_Once);

        // Apply animated transparency (fade in/out)
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, anim_progress);

        // Begin the window. We disable saving settings so it always uses our
        // animated position
        if (ImGui::Begin("Animated Menu", nullptr,
                         ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoCollapse)) {
            ImGui::Text("This window slides down and fades in!");
            ImGui::Separator();

            // Internal window contents
            ImGui::Text("Animation Progress: %.2f", anim_progress);

            if (ImGui::Button("Close Menu")) {
                is_window_open = false;
            }
        }
        ImGui::End();

        // Don't forget to pop the style variable!
        ImGui::PopStyleVar();
    }
}

bool AnimatedCheckbox(const char* label, bool* v) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    // g содержит весь глобальный контекст текущего кадра
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    // 1. Вычисляем геометрию и регистрируем виджет
    const float square_sz = ImGui::GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect total_bb(
        pos, ImVec2(pos.x + square_sz +
                        (label_size.x > 0.0f
                             ? style.ItemInnerSpacing.x + label_size.x
                             : 0.0f),
                    pos.y + label_size.y + style.FramePadding.y * 2.0f));

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) {
        *v = !(*v);
        ImGui::MarkItemEdited(id);
    }

    float hover_t = hovered ? ImMin(g.HoveredIdTimer * 5.0f, 1.0f) : 0.0f;

    float check_anim_t = window->StateStorage.GetFloat(id, *v ? 1.0f : 0.0f);
    check_anim_t =
        ImLerp(check_anim_t, *v ? 1.0f : 0.0f, g.IO.DeltaTime * 12.0f);
    window->StateStorage.SetFloat(id, check_anim_t);

    const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));

    ImVec4 col_bg_vec = ImLerp(style.Colors[ImGuiCol_FrameBg],
                               style.Colors[ImGuiCol_FrameBgHovered], hover_t);
    ImU32 col_bg = ImGui::GetColorU32(col_bg_vec);

    ImGui::RenderFrame(check_bb.Min, check_bb.Max, col_bg, true,
                       style.FrameRounding);

    if (check_anim_t > 0.0f) {
        const float pad = ImMax(1.0f, std::floor(square_sz / 6.0f));
        ImU32 check_col = ImGui::GetColorU32(ImGuiCol_CheckMark);

        int alpha = (int)(255.0f * check_anim_t);
        check_col = (check_col & 0x00FFFFFF) | (alpha << 24);

        ImGui::RenderCheckMark(
            window->DrawList,
            ImVec2(check_bb.Min.x + pad, check_bb.Min.y + pad), check_col,
            square_sz - pad * 2.0f);
    }

    if (label_size.x > 0.0f) {
        ImGui::RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x,
                                 check_bb.Min.y + style.FramePadding.y),
                          label);
    }

    return pressed;
}
