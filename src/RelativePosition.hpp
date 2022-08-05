#pragma once

#include <cassert>

namespace ImGuiGradient {

class RelativePosition {
public:
    explicit RelativePosition(float position)
        : value(position)
    {
        assert_invariants();
    }

    auto get() const -> float { return value; }
    void set(float pos)
    {
        value = pos;
        assert_invariants();
    }

    auto widget(float width) -> bool
    {
        ImGui::SetNextItemWidth(width);
        const float speed{1.f / width};
        return (ImGui::DragFloat(
            "##3",
            &value,
            speed, 0.f, 1.f,
            "%.3f",
            ImGuiSliderFlags_AlwaysClamp
        ));
    }

    friend auto operator<(const RelativePosition& a, const RelativePosition& b) -> bool { return a.get() < b.get(); }
    friend auto operator>(const RelativePosition& a, const RelativePosition& b) -> bool { return a.get() > b.get(); }
    friend auto operator==(const RelativePosition& a, const RelativePosition& b) -> bool { return a.get() == b.get(); }
    friend auto operator!=(const RelativePosition& a, const RelativePosition& b) -> bool { return !(a == b); }

private:
    void assert_invariants() const
    {
        assert(0.f <= value && value <= 1.f && "gdngl"); // TODO(ASG) Error message
    }

private:
    float value{0.f};
};

} // namespace ImGuiGradient