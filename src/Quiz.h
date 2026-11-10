#pragma once

#include "Notes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>

namespace trombone {

struct Settings {
    Clef clef = Clef::Bass;
    int questionCount = 10;
    int lowestMidi = 46;
    int highestMidi = 58;
    std::uint32_t enabledNotes = kAllNotesMask;

    constexpr void normalize() {
        lowestMidi = std::clamp(lowestMidi, kNotes.back().midi, kNotes.front().midi);
        highestMidi = std::clamp(highestMidi, kNotes.back().midi, kNotes.front().midi);
        if (lowestMidi > highestMidi) std::swap(lowestMidi, highestMidi);
        enabledNotes &= kAllNotesMask;
    }

    [[nodiscard]] constexpr bool includesMidi(int midi) const {
        auto normalized = *this;
        normalized.normalize();
        if (midi < normalized.lowestMidi || midi > normalized.highestMidi) return false;
        for (std::size_t i = 0; i < kNotes.size(); ++i) {
            if (kNotes[i].midi == midi) {
                return (normalized.enabledNotes & (std::uint32_t{1} << i)) != 0;
            }
        }
        return false;
    }

    [[nodiscard]] constexpr bool includes(const Note& note) const { return includesMidi(note.midi); }

    [[nodiscard]] constexpr int allowedNoteCount() const {
        int count = 0;
        for (const auto& note : kNotes) {
            if (includes(note)) ++count;
        }
        return count;
    }
};

enum class QuizState { Settings, Question, Feedback, Results };

class Quiz {
public:
    Settings settings;

    bool start(int count, std::uint32_t seed) {
        reset();
        settings.questionCount = std::clamp(count, 1, 100);
        settings.normalize();
        std::array<int, kNotes.size()> eligible{};
        int eligibleCount = 0;
        for (std::size_t i = 0; i < kNotes.size(); ++i) {
            if (settings.includes(kNotes[i])) {
                eligible[static_cast<std::size_t>(eligibleCount++)] = static_cast<int>(i);
            }
        }
        if (eligibleCount == 0) return false;
        count_ = settings.questionCount;
        std::mt19937 random(seed);
        int previous = -1;
        for (int i = 0; i < count_; ++i) {
            // Exclude the previous note whenever the range offers another choice.
            const bool excludePrevious = previous >= 0 && eligibleCount > 1;
            std::uniform_int_distribution<int> distribution(0, eligibleCount - (excludePrevious ? 2 : 1));
            int choice = distribution(random);
            if (excludePrevious && choice >= previous) ++choice;
            questions_[static_cast<std::size_t>(i)] = eligible[static_cast<std::size_t>(choice)];
            previous = choice;
        }
        state_ = QuizState::Question;
        return true;
    }

    bool replay(std::uint32_t seed) { return start(settings.questionCount, seed); }

    void reset() {
        state_ = QuizState::Settings;
        count_ = index_ = score_ = 0;
        selected_ = 1;
        correct_ = false;
    }

    bool select(int position) {
        if (state_ != QuizState::Question || position < 1 || position > 7) return false;
        selected_ = position;
        return true;
    }

    // Return whether the action was processed; lastCorrect() holds its result.
    bool submit() {
        if (state_ != QuizState::Question) return false;
        correct_ = current()->accepts(selected_);
        if (correct_) ++score_;
        state_ = QuizState::Feedback;
        return true;
    }

    bool next() {
        if (state_ != QuizState::Feedback) return false;
        ++index_;
        state_ = index_ < count_ ? QuizState::Question : QuizState::Results;
        selected_ = 1;
        correct_ = false;
        return true;
    }

    [[nodiscard]] const Note* current() const {
        if (state_ != QuizState::Question && state_ != QuizState::Feedback) return nullptr;
        return &kNotes[static_cast<std::size_t>(questions_[static_cast<std::size_t>(index_)])];
    }
    [[nodiscard]] QuizState state() const { return state_; }
    [[nodiscard]] int questionIndex() const { return index_; }
    [[nodiscard]] int questionCount() const { return count_; }
    [[nodiscard]] int selectedPosition() const { return selected_; }
    [[nodiscard]] int score() const { return score_; }
    [[nodiscard]] bool lastCorrect() const { return correct_; }

private:
    std::array<int, 100> questions_{};
    QuizState state_ = QuizState::Settings;
    int count_ = 0;
    int index_ = 0;
    int selected_ = 1;
    int score_ = 0;
    bool correct_ = false;
};

// Seven equal bands, from seventh at the track's top to first at its bottom.
[[nodiscard]] inline int positionFromY(float y, float height) {
    if (!std::isfinite(height) || height <= 0.0f) return 1;
    if (y <= 0.0f) return 7;
    if (y >= height) return 1;
    if (!std::isfinite(y)) return 1;
    return std::clamp(7 - static_cast<int>(static_cast<double>(y) * 7.0 / height), 1, 7);
}

} // namespace trombone
