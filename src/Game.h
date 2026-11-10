#pragma once

#include "Notation.h"
#include "Quiz.h"
#include "olcPixelGameEngine3.h"

#include <memory>
#include <optional>
#include <string>

namespace trombone {

class Game final : public olc::PixelGameEngine {
public:
    Game();
    bool OnUserCreate() override;
    bool OnUserUpdate(float elapsed) override;
    bool OnUserDestroy() override;

private:
    struct Rect {
        olc::vf2d pos, size;
        bool contains(olc::vf2d point) const;
    };
    struct Pointer {
        olc::vf2d pos{};
        bool pressed = false, held = false, released = false;
    };

    void readPointer(float elapsed);
    void syncNotation();
    void drawSettings();
    void drawNotePicker();
    void drawQuestion(bool feedback);
    void drawResults();
    void drawSelector();
    void drawTrombone(Rect bounds);
    void drawClefs(Rect row);
    void text(std::string_view label, olc::vf2d pos, float scale, olc::Pixel color);
    void centered(std::string_view label, Rect box, float scale, olc::Pixel color);
    void wrapped(std::string_view label, Rect box, float scale, olc::Pixel color);
    bool button(std::string_view label, Rect box, bool active = false, bool enabled = true);
    std::uint32_t seed() const;

    Quiz quiz_;
    std::unique_ptr<Notation> notation_;
    Pointer pointer_;
    std::optional<std::uint32_t> touchId_;
    bool dragging_ = false, rangeTab_ = false, notePicker_ = false;
    int notePage_ = 0;
    float mouseCooldown_ = 0, scale_ = 1, width_ = 0, height_ = 0, contentWidth_ = 0, margin_ = 0;
    std::string renderedNotes_, notationError_;
    bool renderedTreble_ = false;
};

} // namespace trombone
