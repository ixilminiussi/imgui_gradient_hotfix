#pragma once

#include <cstddef> // Includes size_t

namespace ImGuiGradient {

/// Controls how the colors are interpolated between two marks.
enum class Interpolation : size_t { // We use size_t so that we can use the WrapMode to index into an array
    /// Linear interpolation between two marks.
    Linear,
    /// Constant color between two marks: it uses the color of the mark on the right.
    Constant,
};

} // namespace ImGuiGradient
