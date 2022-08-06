#include <imgui_gradient/imgui_gradient.hpp>
#include <iterator>
#include "imgui_draw.hpp"
#include "internal.hpp"
#include "random.hpp"
#include "utils.hpp"

namespace ImGuiGradient {

static void tooltip(const char* text)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", text);
        ImGui::EndTooltip();
    }
}

static auto button_with_tooltip(
    const char* label,
    const char* tooltip_message,
    ImVec2      size                = ImVec2{0.f, 0.f},
    bool        should_show_tooltip = true
) -> bool
{
    const bool clicked = ImGui::Button(label, size);
    if (should_show_tooltip)
    {
        tooltip(tooltip_message);
    }
    return clicked;
}

static auto position_mode_combo(WrapMode& position_mode) -> bool
{
    // Take the greater word to choose combo size
    const float size = ImGui::CalcTextSize("Mirror Repeat").x + 30.f;
    ImGui::SetNextItemWidth(size);
    return ImGui::Combo(
        "Position Mode",
        reinterpret_cast<int*>(&position_mode),
        " Clamp\0 Repeat\0 Mirror Clamp\0 Mirror Repeat\0\0"
    );
}

static auto gradient_interpolation_mode(Interpolation& interpolation_mode) -> bool
{
    // Take the greater word to choose combo size
    const float size = ImGui::CalcTextSize("Constant").x + 50.f;
    ImGui::SetNextItemWidth(size);
    return ImGui::Combo(
        "Interpolation Mode",
        reinterpret_cast<int*>(&interpolation_mode),
        " Linear\0 Constant\0\0"
    );
}

static auto delete_button(const float size, bool should_show_tooltip) -> bool
{
    return button_with_tooltip(
        "-",
        "Select a mark to remove it\nor middle click on it\nor drag it down",
        ImVec2{size, size},
        should_show_tooltip
    );
}

static auto add_button(const float size, bool should_show_tooltip) -> bool
{
    return button_with_tooltip(
        "+",
        "Add a mark here\nor click on the gradient to choose its position",
        ImVec2{size, size},
        should_show_tooltip
    );
}

static auto color_button(Mark* selected_mark, bool should_show_tooltip, ImGuiColorEditFlags flags = 0) -> bool
{
    return selected_mark &&
           ImGui::ColorEdit4(
               "##colorpicker1",
               reinterpret_cast<float*>(&selected_mark->color),
               should_show_tooltip
                   ? ImGuiColorEditFlags_NoInputs | flags
                   : ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | flags
           );
}

static auto precise_position(Mark& selected_mark, const float width) -> bool
{
    return selected_mark.position.widget(width);
}

static auto random_mode_box(bool& random_mode, bool should_show_tooltip) -> bool
{
    const bool modified = ImGui::Checkbox("Random Mode", &random_mode);
    if (should_show_tooltip)
    {
        tooltip("Add mark with random color");
    }
    return modified;
}

static auto open_color_picker_popup(Mark& selected_mark, const float popup_size, bool should_show_tooltip, ImGuiColorEditFlags flags = 0) -> bool //
{
    if (ImGui::BeginPopup("SelectedMarkColorPicker"))
    {
        ImGui::SetNextItemWidth(popup_size);
        const bool modified = ImGui::ColorPicker4(
            "##colorpicker2",
            reinterpret_cast<float*>(&selected_mark.color),
            !should_show_tooltip
                ? flags
                : ImGuiColorEditFlags_NoTooltip | flags
        );
        ImGui::EndPopup();
        return modified;
    }
    else
    {
        return false;
    }
}

static void draw_gradient_bar(Gradient& gradient, Interpolation interpolation_mode, ImVec2 gradient_bar_pos, float width, float height)
{
    ImDrawList& draw_list           = *ImGui::GetWindowDrawList();
    const float gradient_botto_barm = gradient_bar_pos.y + height;

    draw_border(draw_list, gradient_bar_pos, ImVec2(gradient_bar_pos.x + width, gradient_botto_barm), internal::border_color());
    if (!gradient.is_empty())
    {
        draw_gradient(gradient, draw_list, interpolation_mode, gradient_bar_pos, gradient_botto_barm, width);
    }
    ImGui::SetCursorScreenPos(ImVec2(gradient_bar_pos.x, gradient_bar_pos.y + height));
}

void handle_interactions_with_hovered_mark(const Mark*& dragging_mark, const Mark*& selected_mark, const Mark*& mark_to_delete, const Mark& hovered_mark)
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        dragging_mark = &hovered_mark;
        selected_mark = &hovered_mark;
    }
    if (ImGui::IsMouseDoubleClicked(ImGuiPopupFlags_MouseButtonLeft))
    {
        ImGui::OpenPopup("SelectedMarkColorPicker");
        selected_mark = &hovered_mark;
    }
    if (ImGui::IsMouseReleased(ImGuiPopupFlags_MouseButtonMiddle))
    {
        // When we middle click to delete a non selected mark it is impossible to remove this mark in the loop
        mark_to_delete = &hovered_mark;
    }
}

auto GradientWidget::draw_gradient_marks(
    const Mark*   mark_to_delete,
    const ImVec2& gradient_bar_pos,
    float width, float height
) -> bool
{
    ImDrawList& draw_list         = *ImGui::GetWindowDrawList();
    bool        hitbox_is_hovered = false;
    for (const Mark& mark_hovered : state.gradient.get_marks())
    {
        if (state.mark_to_hide != &mark_hovered)
        {
            mark_invisble_hitbox(
                draw_list,
                gradient_bar_pos + ImVec2(mark_hovered.position.get() * width, height),
                ImGui::ColorConvertFloat4ToU32(mark_hovered.color),
                height,
                state.selected_mark == &mark_hovered
            );
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
            {
                hitbox_is_hovered = true;
                const Mark* const_dragging_mark{state.dragging_mark};
                const Mark* const_selected_mark{state.selected_mark};
                handle_interactions_with_hovered_mark(const_dragging_mark, const_selected_mark, mark_to_delete, mark_hovered);
                if (const_selected_mark)
                {
                    state.selected_mark = const_cast<Mark*>(const_selected_mark);
                }
                if (const_dragging_mark)
                {
                    state.dragging_mark = const_cast<Mark*>(const_dragging_mark);
                }
            }
        }
    }
    static constexpr float space_between_gradient_marks_and_options = 20.f;
    ImGui::SetCursorScreenPos(ImVec2(gradient_bar_pos.x, gradient_bar_pos.y + height + space_between_gradient_marks_and_options));
    return hitbox_is_hovered;
}

auto position_where_to_add_next_mark(Gradient& gradient) -> float
{
    if (gradient.is_empty())
    {
        return 0.f;
    }
    else if (gradient.get_marks().size() == 1)
    {
        return (gradient.get_marks().begin()->position.get() > 1.f - gradient.get_marks().begin()->position.get()) ? 0.f : 1.f;
    }
    else
    {
        float max_value_mark_position     = 0;
        float max_value_between_two_marks = gradient.get_marks().begin()->position.get();
        for (auto markIt = gradient.get_marks().begin(); markIt != std::prev(gradient.get_marks().end()); ++markIt)
        {
            const Mark& mark = *markIt;
            if (max_value_between_two_marks < abs(std::next(markIt)->position.get() - mark.position.get()))
            {
                max_value_mark_position     = mark.position.get();
                max_value_between_two_marks = abs(std::next(markIt)->position.get() - max_value_mark_position);
            }
        }
        if (max_value_between_two_marks < abs(1.f - std::prev(gradient.get_marks().end())->position.get()))
        {
            max_value_mark_position     = std::prev(gradient.get_marks().end())->position.get();
            max_value_between_two_marks = abs(1.f - max_value_mark_position);
        }
        return max_value_mark_position + max_value_between_two_marks / 2.f;
    }
}

auto GradientWidget::mouse_dragging(const float gradient_bar_bottom, float width, float gradient_bar_pos_x, const Settings& settings) -> bool
{
    bool dragging = false;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && state.dragging_mark)
    {
        state.dragging_mark = nullptr;
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && state.dragging_mark)
    {
        const float map = ImClamp((ImGui::GetIO().MousePos.x - gradient_bar_pos_x) / width, 0.f, 1.f);
        if (state.dragging_mark->position.get() != map)
        {
            state.dragging_mark->position.set(map);
            state.gradient.set_mark_position(*state.dragging_mark, state.dragging_mark->position);
            dragging = true;
        }
        if (!(settings.flags & Flag::NoDragDowntoDelete))
        { // hide dragging mark when mouse under gradient bar
            float diffY = ImGui::GetIO().MousePos.y - gradient_bar_bottom;
            if (diffY >= settings.delete_mark_by_dragging_down_sensibility)
            {
                state.mark_to_hide = state.dragging_mark;
            }
            // do not hide it anymore when mouse on gradient bar
            if (state.mark_to_hide && diffY <= settings.delete_mark_by_dragging_down_sensibility)
            {
                state.dragging_mark = state.mark_to_hide;
                state.mark_to_hide  = nullptr;
            }
        }
    }
    return dragging;
}

static auto random_color(std::default_random_engine& generator) -> ImVec4
{
    const auto color = ImVec4{Utils::random(generator), Utils::random(generator), Utils::random(generator), 1.f};
    return color;
}

auto GradientWidget::add_mark(const float position, std::default_random_engine& generator) -> bool
{
    const auto   relative_pos = RelativePosition{[&] {
        switch (position_mode)
        {
        case WrapMode::Clamp:
        {
            return ImClamp(position, 0.f, 1.f);
        }
        case WrapMode::Repeat:
        {
            return Utils::repeat_position(position);
        }
        case WrapMode::MirrorClamp:
        {
            return Utils::mirror_clamp_position(position);
        }
        case WrapMode::MirrorRepeat:
        {
            return Utils::mirror_repeat_position(position);
        }
        default:
            assert(false && "[Gradient::get_color_at] Invalid enum value");
            return 0.5f;
        }
    }()};
    const ImVec4 new_mark_col = (random_mode) ? random_color(generator) : state.gradient.compute_color_at(relative_pos);
    return (state.selected_mark = &state.gradient.add_mark(Mark{RelativePosition{relative_pos}, new_mark_col}));
}

auto GradientWidget::widget_with_chosen_rnd(
    const char*                 label,
    std::default_random_engine& generator,
    const Settings&             settings
) -> bool
{
    if (!(settings.flags & Flag::NoLabel))
    {
        ImGui::Text("%s", label);
        ImGui::Dummy(ImVec2{0.f, 1.5f});
    }

    const ImVec2 gradient_bar_pos    = internal::gradient_position(settings.horizontal_margin);
    const float  width               = std::max(1.f, ImGui::GetContentRegionAvail().x - 2.f * settings.horizontal_margin);
    const float  gradient_bar_bottom = gradient_bar_pos.y + settings.gradient_editor_height;

    ImGui::BeginGroup();
    ImGui::InvisibleButton("gradient_editor", ImVec2(width, settings.gradient_editor_height));
    draw_gradient_bar(state.gradient, interpolation_mode, gradient_bar_pos, width, settings.gradient_editor_height);

    Mark*      mark_to_delete         = nullptr;
    const bool add_mark_possible      = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mark_hitbox_is_hovered = draw_gradient_marks(mark_to_delete, gradient_bar_pos, width, settings.gradient_editor_height);

    bool modified = false;
    if (add_mark_possible && !mark_hitbox_is_hovered)
    {
        modified = add_mark((ImGui::GetIO().MousePos.x - gradient_bar_pos.x) / width, generator);
        ImGui::OpenPopup("SelectedMarkColorPicker");
    }

    modified |= mouse_dragging(gradient_bar_bottom, width, gradient_bar_pos.x, settings);
    if (!(settings.flags & Flag::NoDragDowntoDelete))
    { // If mouse released and there is still a mark hidden, then it become a mark to delete
        if (state.mark_to_hide && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (state.dragging_mark && *state.dragging_mark == *state.mark_to_hide)
            {
                state.dragging_mark = nullptr;
            }
            mark_to_delete     = state.mark_to_hide;
            state.mark_to_hide = nullptr;
            modified |= true;
        }
    }
    // Remove mark_to_delete if it exists
    if (mark_to_delete)
    {
        if (state.selected_mark && *state.selected_mark == *mark_to_delete)
        {
            state.selected_mark = nullptr;
        }
        state.gradient.remove_mark(*mark_to_delete);
        modified |= true;
    }
    ImGui::EndGroup();
    const bool no_tooltip           = !(settings.flags & Flag::NoTooltip);
    const bool remove_button_exists = !(settings.flags & Flag::NoRemoveButton);
    if (!state.gradient.is_empty())
    {
        if (((remove_button_exists &&
              delete_button(internal::button_height(), no_tooltip)) ||
             ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) &&
            state.selected_mark)
        {
            state.gradient.remove_mark(*state.selected_mark);
            state.selected_mark = nullptr;
            modified |= true;
        }
    }
    const bool add_button_exists = !(settings.flags & Flag::NoAddButton);
    if (add_button_exists)
    {
        if (remove_button_exists && !state.gradient.is_empty())
        {
            ImGui::SameLine();
        }
        if (add_button(internal::button_height(), no_tooltip))
        {
            // Add a mark where there is the greater space in the gradient
            modified = add_mark(position_where_to_add_next_mark(state.gradient), generator);
        }
    }
    const bool color_edit_exists = !(settings.flags & Flag::NoColorEdit);
    if (color_edit_exists)
    {
        if ((remove_button_exists || add_button_exists) && state.selected_mark)
        {
            ImGui::SameLine();
        }
        modified |= color_button(state.selected_mark, no_tooltip, settings.color_flags);
    }
    if (!(settings.flags & Flag::NoPositionSlider))
    {
        if ((remove_button_exists || add_button_exists || color_edit_exists) && state.selected_mark)
        {
            ImGui::SameLine();
        }

        if (state.selected_mark && precise_position(*state.selected_mark, width * .25f))
        {
            state.gradient.set_mark_position(*state.selected_mark, state.selected_mark->position);
            modified = true;
        }
    }

    const bool interpolation_combo_exists = !(settings.flags & Flag::NoInterpolationCombo);
    if (interpolation_combo_exists)
    {
        modified |= gradient_interpolation_mode(interpolation_mode);
    }
    const bool position_mode_combo_exists = !(settings.flags & Flag::NoWrapModeCombo);
    if (position_mode_combo_exists)
    {
        if (interpolation_combo_exists)
        {
            ImGui::SameLine();
        }
        modified |= position_mode_combo(position_mode);
    }

    if (!(settings.flags & Flag::NoRandomModeChange))
    {
        if (position_mode_combo_exists || interpolation_combo_exists)
        {
            ImGui::SameLine();
        }
        modified |= random_mode_box(random_mode, no_tooltip);
    }

    if (!(settings.flags & Flag::NoResetButton))
    {
        if (ImGui::Button("Reset"))
        {
            state = State{};
            modified |= true;
        }
    }

    if (state.selected_mark)
    {
        modified |= open_color_picker_popup(*state.selected_mark, internal::button_height() * 12.f, no_tooltip, settings.flags);
    }

    if (!(settings.flags & Flag::NoBorder))
    {
        float y_space_over_bar = 8.f;
        if (!(settings.flags & Flag::NoLabel))
        {
            y_space_over_bar = ImGui::CalcTextSize(label).y * 2.3f;
        }
        float number_of_line_under_bar = 0.f;
        if (!(settings.flags & Flag::NoRandomModeChange) ||
            !(settings.flags & Flag::NoCombo))
        {
            number_of_line_under_bar += 1.f;
        }
        if (!(settings.flags & Flag::NoResetButton))
        {
            number_of_line_under_bar += 1.f;
        }
        if (!(settings.flags & Flag::NoAddButton) ||
            !(settings.flags & Flag::NoRemoveButton) ||
            !(settings.flags & Flag::NoPositionSlider) ||
            !(settings.flags & Flag::NoColorEdit))
        {
            number_of_line_under_bar += 1.f;
        }
        ImDrawList& draw_list = *ImGui::GetWindowDrawList();

        const float y_space_under_bar = gradient_bar_bottom + internal::button_height() * number_of_line_under_bar;
        draw_border(
            draw_list,
            gradient_bar_pos - ImVec2(settings.horizontal_margin + 4.f, y_space_over_bar),
            ImVec2(gradient_bar_pos.x + width + settings.horizontal_margin + 4.f, y_space_under_bar * 1.25f),
            internal::border_color()
        );
    }

    return modified;
}

auto GradientWidget::widget(const char* label, const Settings& settings) -> bool
{
    static std::default_random_engine generator{std::random_device{}()};
    return widget_with_chosen_rnd(label, generator, settings);
}

}; // namespace ImGuiGradient
