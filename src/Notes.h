#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace trombone {

enum class Clef { Bass, Treble };

struct Note {
    std::string_view name;
    std::string_view ldp;
    int midi;
    int referencePosition;
    std::uint8_t acceptedMask;
    std::string_view compactLabel{};

    [[nodiscard]] constexpr std::string_view compactName() const {
        return compactLabel.empty() ? name : compactLabel;
    }

    [[nodiscard]] constexpr std::string_view displayName() const {
        auto display = name;
        if (!display.empty() && display.back() >= '0' && display.back() <= '9') {
            display.remove_suffix(1);
            if (display.ends_with(' ')) display.remove_suffix(1);
        }
        return display;
    }

    [[nodiscard]] constexpr bool accepts(int position) const {
        return position >= 1 && position <= 7 &&
               (acceptedMask & (1u << (position - 1))) != 0;
    }
};

// Concert pitches, shared by both clefs. Ordinary second to sixth partials
// only; see docs/validation/NOTE-SOURCES.md for accepted alternate positions.
inline constexpr std::array<Note, 17> kNotes{{
    {"D4",        "d4",  62, 1, 0b0001001, "D4"},  // 1, 4
    {"C-sharp 4", "+c4", 61, 2, 0b0010010, "C#4"}, // 2, 5
    {"C4",        "c4",  60, 3, 0b0100100, "C4"},  // 3, 6
    {"B3",        "b3",  59, 4, 0b1001000, "B3"},  // 4, 7
    {"B-flat 3",  "-b3", 58, 5, 0b0010001, "Bb3"}, // 5, 1
    {"A3",        "a3",  57, 6, 0b0100010, "A3"},  // 6, 2
    {"A-flat 3",  "-a3", 56, 7, 0b1000100, "Ab3"}, // 7, 3
    {"G3",        "g3",  55, 4, 0b0001000, "G3"},  // 4
    {"F-sharp 3", "+f3", 54, 5, 0b0010000, "F#3"}, // 5
    {"F3",        "f3",  53, 1, 0b0100001, "F3"},  // 1, 6
    {"E3",        "e3",  52, 2, 0b1000010, "E3"},  // 2, 7
    {"E-flat 3",  "-e3", 51, 3, 0b0000100, "Eb3"}, // 3
    {"D3",        "d3",  50, 4, 0b0001000, "D3"},  // 4
    {"C-sharp 3", "+c3", 49, 5, 0b0010000, "C#3"}, // 5
    {"C3",        "c3",  48, 6, 0b0100000, "C3"},  // 6
    {"B2",        "b2",  47, 7, 0b1000000, "B2"},  // 7
    {"B-flat 2",  "-b2", 46, 1, 0b0000001, "Bb2"}, // 1
}};

// Bit i enables kNotes[i], independently of notes with the same pitch class.
inline constexpr std::uint32_t kAllNotesMask = (std::uint32_t{1} << kNotes.size()) - 1;

} // namespace trombone
