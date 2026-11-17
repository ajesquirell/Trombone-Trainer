#pragma once

#include <algorithm>
#include <cmath>

namespace trombone {

inline constexpr float kInstrumentDesignWidth = 1000.0f;
inline constexpr float kInstrumentDesignHeight = 180.0f;
inline constexpr float kSlidePositionStep = 50.0f;

struct InstrumentGeometry {
    float originX = 0, originY = 0, scale = 0;

    [[nodiscard]] static InstrumentGeometry make(float x, float y, float w, float h) {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) ||
            !std::isfinite(h) || w <= 0 || h <= 0) return {};
        const float scale = std::min(w / kInstrumentDesignWidth, h / kInstrumentDesignHeight);
        return {x + (w - kInstrumentDesignWidth * scale) * 0.5f,
                y + (h - kInstrumentDesignHeight * scale) * 0.5f, scale};
    }

    [[nodiscard]] float stepPixels() const { return kSlidePositionStep * scale; }

    [[nodiscard]] bool hitsSlide(float px, float py, int position) const {
        if (!std::isfinite(scale) || scale <= 0 || !std::isfinite(px) ||
            !std::isfinite(py)) return false;
        const float travel = kSlidePositionStep * (std::clamp(position, 1, 7) - 1);
        const float left = originX + (250 + travel) * scale - 12;
        const float right = originX + (674 + travel) * scale + 12;
        const float centerY = originY + 120 * scale;
        const float halfHeight = std::max(24 * scale + 12, 22.0f);
        return px >= left && px <= right && py >= centerY - halfHeight &&
               py <= centerY + halfHeight;
    }
};

struct SlideDrag {
    enum class Mode { None, Column, Slide };
    Mode mode = Mode::None;

    void beginColumn() { mode = Mode::Column; }

    void beginSlide(float pointerX, int position, float stepPixels) {
        position_ = initialPosition_ = std::clamp(position, 1, 7);
        anchorX_ = pointerX;
        stepPixels_ = stepPixels;
        mode = std::isfinite(pointerX) && std::isfinite(stepPixels) && stepPixels > 0
                   ? Mode::Slide : Mode::None;
    }

    [[nodiscard]] int updateSlide(float pointerX) {
        if (mode != Mode::Slide || !std::isfinite(pointerX)) return position_;
        const double target = std::clamp(initialPosition_ +
            (static_cast<double>(pointerX) - anchorX_) / stepPixels_, 1.0, 7.0);
        // Separate forward and return thresholds keep boundary jitter stable.
        while (position_ < 7 && target >= position_ + 0.6) ++position_;
        while (position_ > 1 && target <= position_ - 0.6) --position_;
        return position_;
    }

    void cancel() { mode = Mode::None; }

private:
    float anchorX_ = 0, stepPixels_ = 1;
    int initialPosition_ = 1, position_ = 1;
};

} // namespace trombone
