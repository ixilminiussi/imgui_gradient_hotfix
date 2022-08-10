#pragma once

#include <imgui/imgui.h>
#include <cassert>
#include "WrapMode.hpp"

namespace ImGuiGradient {

/// Represents a number between 0 and 1.
class RelativePosition {
public:
    /// `position` must be between 0 and 1.
    explicit RelativePosition(float position)
        : value{position}
    {
        assert_invariants();
    }

    /// Returns a number between 0 and 1.
    auto get() const -> float { return value; }

    /// `position` must be between 0 and 1.
    void set(float position)
    {
        value = position;
        assert_invariants();
    }

    auto imgui_widget(float width) -> bool
    {
        ImGui::SetNextItemWidth(width);
        return ImGui::DragFloat(
            "##3", // TODO(ASG) Receive this as a parameter (with a default value). If we want to render two RelativePosition in the same window this will be a problem if they have the same label.
            &value,
            .0001f,   /* speed */
            0.f, 1.f, /* min and max */
            "%.4f",
            ImGuiSliderFlags_AlwaysClamp
        );
    }

    friend auto operator<(const RelativePosition& a, const RelativePosition& b) -> bool { return a.get() < b.get(); }
    friend auto operator>(const RelativePosition& a, const RelativePosition& b) -> bool { return a.get() > b.get(); }
    friend auto operator==(const RelativePosition& a, const RelativePosition& b) -> bool { return a.get() == b.get(); }
    friend auto operator!=(const RelativePosition& a, const RelativePosition& b) -> bool { return !(a == b); }

private:
    void assert_invariants() const
    {
        assert(0.f <= value && value <= 1.f && "RelativePosition value should be between 0.f and 1.f");
    }

private:
    float value{0.f};
};

// TODO(ASG) I changed my mind, this will be better suited as a constructor O:)
auto make_relative_position(float position, WrapMode wrap_mode) -> RelativePosition;

} // namespace ImGuiGradient