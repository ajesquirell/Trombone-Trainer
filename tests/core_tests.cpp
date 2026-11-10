#include "Notes.h"
#include "Quiz.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

using namespace trombone;

namespace {
int checks = 0;
int failures = 0;

void check(bool condition, std::string_view description) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

void notePool() {
    struct Expected {
        std::string_view name;
        std::string_view ldp;
        int midi;
        int reference;
        int alternate;
    };
    // Independently transcribed from the concert-pitch, open-slide chart in
    // Matt Waters, basstrombone.co.uk/resources/trombone-slide-position-chart/.
    // See docs/validation/NOTE-SOURCES.md for spelling and reference choices.
    constexpr std::array expected{
        Expected{"D4", "d4", 62, 1, 4},
        Expected{"C-sharp 4", "+c4", 61, 2, 5},
        Expected{"C4", "c4", 60, 3, 6},
        Expected{"B3", "b3", 59, 4, 7},
        Expected{"B-flat 3", "-b3", 58, 5, 1},
        Expected{"A3", "a3", 57, 6, 2},
        Expected{"A-flat 3", "-a3", 56, 7, 3},
        Expected{"G3", "g3", 55, 4, 0},
        Expected{"F-sharp 3", "+f3", 54, 5, 0},
        Expected{"F3", "f3", 53, 1, 6},
        Expected{"E3", "e3", 52, 2, 7},
        Expected{"E-flat 3", "-e3", 51, 3, 0},
        Expected{"D3", "d3", 50, 4, 0},
        Expected{"C-sharp 3", "+c3", 49, 5, 0},
        Expected{"C3", "c3", 48, 6, 0},
        Expected{"B2", "b2", 47, 7, 0},
        Expected{"B-flat 2", "-b2", 46, 1, 0},
    };
    check(kNotes.size() == 17, "all seventeen chromatic pitches from Bb2 through D4 are playable");
    for (std::size_t i = 0; i < expected.size() && i < kNotes.size(); ++i) {
        const auto& note = kNotes[i];
        const auto& want = expected[i];
        check(note.name == want.name, "feedback identifies the exact note and octave");
        check(note.ldp == want.ldp, "engraving identifies the exact pitch and octave");
        check(note.midi == want.midi, "note uses concert pitch MIDI octave numbering");
        check(note.referencePosition == want.reference, "each reference slide position is represented");
        for (int position = 0; position <= 8; ++position) {
            check(note.accepts(position) == (position == want.reference || (want.alternate != 0 && position == want.alternate)),
                  "accept standard reference and alternate, reject every other position");
        }
    }
    check(!kNotes[0].accepts(7), "D4 seventh-partial position is outside the beginner pool");
}

void displayNames() {
    constexpr std::array expected{
        "D", "C-sharp", "C", "B", "B-flat", "A", "A-flat", "G", "F-sharp",
        "F", "E", "E-flat", "D", "C-sharp", "C", "B", "B-flat"};
    constexpr std::array compact{
        "D4", "C#4", "C4", "B3", "Bb3", "A3", "Ab3", "G3", "F#3",
        "F3", "E3", "Eb3", "D3", "C#3", "C3", "B2", "Bb2"};
    for (std::size_t i = 0; i < kNotes.size(); ++i) {
        check(kNotes[i].displayName() == expected[i], "display name omits the octave and its space");
        check(kNotes[i].compactName() == compact[i], "picker name retains a compact accidental and exact octave");
    }
    constexpr auto sharp = kNotes[1].displayName();
    check(sharp == "C-sharp" && kNotes[1].name == "C-sharp 4" &&
              kNotes[1].ldp == "+c4" && kNotes[1].midi == 61,
          "display formatting preserves the original exact pitch data");
    constexpr Note compactFlat{"B-flat3", "-b3", 58, 5, 0b0010001};
    check(compactFlat.displayName() == "B-flat", "display name also handles an octave without a space");
}

void roundTransitions() {
    Quiz quiz;
    check(quiz.settings.clef == Clef::Bass && quiz.settings.questionCount == 10,
          "settings default to bass clef and ten questions");
    check(quiz.state() == QuizState::Settings && quiz.current() == nullptr,
          "new quiz begins at settings without a question");
    check(!quiz.select(4) && !quiz.submit() && !quiz.next(), "settings reject question actions");
    quiz.settings.clef = Clef::Treble;
    quiz.start(2, 17);
    check(quiz.state() == QuizState::Question && quiz.current() != nullptr,
          "start opens a question");
    check(quiz.questionIndex() == 0 && quiz.questionCount() == 2 && quiz.score() == 0,
          "start resets progress and score");
    check(quiz.selectedPosition() == 1, "new question begins with slide in first position");
    check(!quiz.next(), "cannot skip a question before submitting");
    check(!quiz.select(0) && !quiz.select(8) && quiz.selectedPosition() == 1,
          "invalid positions cannot enter quiz state");
    const auto* first = quiz.current();
    check(quiz.select(first->referencePosition), "question accepts a valid selected position");
    check(quiz.submit(), "submission is processed");
    check(quiz.lastCorrect() && quiz.score() == 1 && quiz.state() == QuizState::Feedback,
          "correct answer earns one point and opens feedback");
    check(quiz.current() == first, "feedback retains the answered note");
    check(!quiz.submit() && quiz.score() == 1, "duplicate submit cannot earn another point");
    check(!quiz.select(7), "feedback locks the submitted position");
    check(quiz.next() && quiz.state() == QuizState::Question && quiz.questionIndex() == 1,
          "next advances only after feedback");
    check(quiz.selectedPosition() == 1, "next question resets slide selection");
    int wrong = 1;
    while (quiz.current()->accepts(wrong)) ++wrong;
    quiz.select(wrong);
    quiz.submit();
    check(!quiz.lastCorrect() && quiz.score() == 1 && quiz.state() == QuizState::Feedback,
          "wrong answer earns no point and still receives feedback");
    check(quiz.next() && quiz.state() == QuizState::Results && quiz.current() == nullptr,
          "last feedback advances to results");
    check(quiz.score() == 1 && quiz.questionCount() == 2,
          "results preserve earned score and round length");
    check(!quiz.next() && !quiz.submit() && !quiz.select(2), "results reject question actions");
    quiz.replay(18);
    check(quiz.state() == QuizState::Question && quiz.score() == 0 && quiz.questionIndex() == 0,
          "replay starts a fresh round");
    check(quiz.questionCount() == 2 && quiz.settings.clef == Clef::Treble,
          "replay retains length and clef settings");
    quiz.reset();
    check(quiz.state() == QuizState::Settings && quiz.current() == nullptr && quiz.score() == 0,
          "returning to settings clears the round");
    check(quiz.settings.questionCount == 2 && quiz.settings.clef == Clef::Treble,
          "returning to settings preserves user choices");
}

void questionGeneration() {
    Quiz left;
    Quiz right;
    Quiz other;
    Quiz wrong;
    for (auto* quiz : {&left, &right, &other, &wrong}) {
        quiz->settings.lowestMidi = 46;
        quiz->settings.highestMidi = 62;
    }
    left.start(100, 123456);
    right.start(100, 123456);
    other.start(100, 654321);
    wrong.start(100, 123456);
    const Note* previous = nullptr;
    std::array<bool, 17> seen{};
    bool differentSeedChangedQuestions = false;
    for (int i = 0; i < 100; ++i) {
        const auto* note = left.current();
        check(note && right.current() && note->midi == right.current()->midi,
              "seed reproduces each question in a round");
        check(note != previous, "random sequence never repeats an adjacent note");
        if (!note) break;
        check(note->referencePosition >= 1 && note->referencePosition <= 7,
              "drawn note has a usable reference position");
        if (note->referencePosition < 1 || note->referencePosition > 7) break;
        differentSeedChangedQuestions |= note->midi != other.current()->midi;
        seen[static_cast<std::size_t>(note->midi - 46)] = true;
        previous = note;
        left.select(note->referencePosition);
        int accepted = note->referencePosition;
        for (int position = 1; position <= 7; ++position) {
            if (position != note->referencePosition && note->accepts(position)) accepted = position;
        }
        right.select(accepted);
        int incorrect = 1;
        while (wrong.current()->accepts(incorrect)) ++incorrect;
        wrong.select(incorrect);
        left.submit();
        right.submit();
        wrong.submit();
        check(right.lastCorrect(), "quiz scores every drawn pitch using an alternate where one exists");
        check(!wrong.lastCorrect(), "quiz rejects incorrect positions for every drawn pitch");
        other.submit();
        left.next();
        right.next();
        other.next();
        wrong.next();
    }
    for (bool present : seen) check(present, "seeded round can draw every note");
    check(left.state() == QuizState::Results && left.score() == 100,
          "all correct answers yield a perfect score");
    check(right.score() == 100 && wrong.score() == 0,
          "alternate answers score fully and wrong answers score zero");
    check(differentSeedChangedQuestions, "changing the seed can change the question sequence");
    left.start(0, 1);
    check(left.questionCount() == 1 && left.settings.questionCount == 1,
          "zero count is clamped to one");
    left.start(-500, 1);
    check(left.questionCount() == 1, "negative count is clamped to one");
    left.start(101, 1);
    check(left.questionCount() == 100 && left.settings.questionCount == 100,
          "large count is clamped to one hundred");
    left.start(1, 1);
    check(left.questionCount() == 1 && left.score() == 0, "start replaces the previous round");
}

void noteRanges() {
    Quiz quiz;
    check(quiz.settings.lowestMidi == 46 && quiz.settings.highestMidi == 58,
          "default range is the chromatic octave from Bb2 to Bb3");
    quiz.settings.lowestMidi = 58;
    quiz.settings.highestMidi = 60;
    quiz.start(100, 123456);
    std::array<bool, 3> seen{};
    const Note* previous = nullptr;
    for (int i = 0; i < 100; ++i) {
        const auto* note = quiz.current();
        const bool inRange = note && note->midi >= 58 && note->midi <= 60;
        check(inRange, "questions stay inside the selected inclusive range");
        if (!inRange) break;
        seen[static_cast<std::size_t>(note->midi - 58)] = true;
        check(note != previous, "a range with multiple notes avoids adjacent repeats");
        previous = note;
        quiz.select(note->referencePosition);
        quiz.submit();
        quiz.next();
    }
    check(seen[0] && seen[1] && seen[2], "both selected bounds and the note between them can be drawn");
    check(quiz.state() == QuizState::Results && quiz.score() == 100,
          "a filtered round retains normal question count and scoring");

    quiz.settings.lowestMidi = 56;
    quiz.settings.highestMidi = 56;
    quiz.start(6, 17);
    for (int i = 0; i < 6; ++i) {
        check(quiz.current() && quiz.current()->midi == 56,
              "a singleton range safely repeats its only selected note");
        quiz.select(i % 2 == 0 ? 7 : 1);
        check(quiz.submit() && quiz.lastCorrect() == (i % 2 == 0),
              "a singleton round scores correct and incorrect answers normally");
        quiz.next();
    }
    check(quiz.state() == QuizState::Results && quiz.questionCount() == 6 && quiz.score() == 3,
          "a singleton round finishes at its selected count with the earned score");

    quiz.settings.lowestMidi = 60;
    quiz.settings.highestMidi = 61;
    quiz.start(10, 1);
    int previousMidi = -1;
    for (int i = 0; i < 10; ++i) {
        const auto* note = quiz.current();
        check(note && (note->midi == 60 || note->midi == 61),
              "a two-note range includes only its lower and upper bounds");
        if (!note) break;
        check(note->midi != previousMidi, "a two-note range alternates on every question");
        previousMidi = note->midi;
        quiz.submit();
        quiz.next();
    }
}

void normalizedRanges() {
    struct Range {
        int low;
        int high;
        int expectedLow;
        int expectedHigh;
    };
    constexpr std::array ranges{
        Range{-500, 500, 46, 62},
        Range{61, 57, 57, 61},
        Range{500, -500, 46, 62},
        Range{0, 40, 46, 46},
        Range{80, 100, 62, 62},
        Range{59, 59, 59, 59},
        Range{59, 500, 59, 62},
    };
    Quiz quiz;
    for (const auto& range : ranges) {
        quiz.settings.lowestMidi = range.low;
        quiz.settings.highestMidi = range.high;
        quiz.start(1, 17);
        check(quiz.settings.lowestMidi == range.expectedLow &&
                  quiz.settings.highestMidi == range.expectedHigh,
              "start clamps bounds to the playable pool and orders reversed bounds");
        check(quiz.current() && quiz.current()->midi >= range.expectedLow &&
                  quiz.current()->midi <= range.expectedHigh,
              "normalized bounds always produce an eligible question");
    }
    quiz.settings.clef = Clef::Treble;
    quiz.settings.lowestMidi = 61;
    quiz.settings.highestMidi = 58;
    quiz.start(8, 2);
    quiz.select(quiz.current()->referencePosition);
    quiz.submit();
    quiz.replay(3);
    check(quiz.settings.lowestMidi == 58 && quiz.settings.highestMidi == 61 &&
              quiz.settings.clef == Clef::Treble && quiz.questionCount() == 8,
          "replay preserves the normalized selected range, clef, and count");
    check(quiz.state() == QuizState::Question && quiz.score() == 0 && quiz.questionIndex() == 0,
          "replay of a filtered round resets its progress");
    for (int i = 0; i < 8; ++i) {
        check(quiz.current() && quiz.current()->midi >= 58 && quiz.current()->midi <= 61,
              "replay continues to draw only inside the selected range");
        quiz.submit();
        quiz.next();
    }
    quiz.reset();
    check(quiz.settings.lowestMidi == 58 && quiz.settings.highestMidi == 61 &&
              quiz.settings.clef == Clef::Treble && quiz.settings.questionCount == 8,
          "returning to settings preserves normalized range, clef, and count");
}


// These checks catch treating the mask as a pitch-class filter, forgetting the
// selected range, and attempting to sample an empty distribution.
void exactPitchSelection() {
    Settings settings;
    check(settings.enabledNotes == kAllNotesMask && kAllNotesMask == 0x1ffffu,
          "the mask has one enabled bit for each pool note and no other bits");
    check(settings.allowedNoteCount() == 13 && settings.includesMidi(46) &&
              settings.includesMidi(58) && !settings.includesMidi(59),
          "default selection includes the full Bb2 to Bb3 chromatic octave");
    check(settings.includes(kNotes.back()) && !settings.includes(kNotes.front()) &&
              !settings.includesMidi(45) && !settings.includesMidi(63),
          "selection rejects pitches outside the pool and selected bounds");
    settings.lowestMidi = 62;
    settings.highestMidi = 46;
    settings.enabledNotes = (1u << 0) | (1u << 12) | (1u << 16);
    check(settings.allowedNoteCount() == 3 && settings.includesMidi(62) &&
              settings.includesMidi(50) && settings.includesMidi(46) && !settings.includesMidi(58),
          "reversed bounds and independent exact-pitch bits are counted correctly");
    settings.enabledNotes = ~std::uint32_t{0};
    settings.lowestMidi = 500;
    settings.highestMidi = -500;
    check(settings.allowedNoteCount() == 17 && settings.includesMidi(46) && settings.includesMidi(62),
          "counting and membership normalize out-of-bounds reversed limits before use");
    settings.normalize();
    check(settings.lowestMidi == 46 && settings.highestMidi == 62 &&
              settings.enabledNotes == kAllNotesMask,
          "normalization stores bounded limits and discards unknown mask bits");

    Quiz quiz;
    quiz.settings.lowestMidi = 46;
    quiz.settings.highestMidi = 62;
    // Bb2 and Bb3 have the same pitch class but remain independent choices.
    quiz.settings.enabledNotes = (1u << 4) | (1u << 16);
    check(quiz.start(100, 42), "two enabled notes can start a round");
    int previousMidi = -1;
    for (int i = 0; i < 100; ++i) {
        const auto* note = quiz.current();
        const bool allowed = note && (note->midi == 46 || note->midi == 58);
        check(allowed, "a sparse mask draws only its exact selected pitches");
        if (!allowed) break;
        check(note->midi != previousMidi, "two enabled pitches alternate without adjacent repeats");
        previousMidi = note->midi;
        quiz.submit();
        quiz.next();
    }
    quiz.settings.highestMidi = 57;
    check(quiz.settings.allowedNoteCount() == 1 && quiz.start(5, 71),
          "the range filters enabled notes before question generation");
    for (int i = 0; i < 5; ++i) {
        check(quiz.current() && quiz.current()->midi == 46,
              "one enabled pitch repeats safely even with another enabled outside the range");
        quiz.submit();
        quiz.next();
    }
    const auto mask = quiz.settings.enabledNotes;
    check(quiz.replay(72) && quiz.settings.enabledNotes == mask &&
              quiz.settings.lowestMidi == 46 && quiz.settings.highestMidi == 57 &&
              quiz.current() && quiz.current()->midi == 46,
          "replay preserves both the exact pitch mask and selected range");

    quiz.settings.enabledNotes = 1u << 0; // D4 is outside the current range.
    check(quiz.settings.allowedNoteCount() == 0 && !quiz.start(7, 73),
          "an enabled note outside the range does not silently replace an empty selection");
    check(quiz.state() == QuizState::Settings && quiz.current() == nullptr &&
              quiz.questionCount() == 0 && quiz.score() == 0 && quiz.questionIndex() == 0,
          "an empty selection clears the old round and stays at settings");
    check(!quiz.select(1) && !quiz.submit() && !quiz.next(),
          "an empty selection cannot produce question actions");
    check(!quiz.replay(74) && quiz.settings.enabledNotes == (1u << 0) &&
              quiz.settings.questionCount == 7 && quiz.state() == QuizState::Settings,
          "replay of an empty selection fails while preserving the user's choices");
    quiz.settings.enabledNotes = 0;
    check(quiz.settings.allowedNoteCount() == 0 && !quiz.start(3, 75),
          "disabling every pitch safely prevents the round from starting");
    quiz.settings.enabledNotes = 1u << 31;
    check(quiz.settings.allowedNoteCount() == 0 && !quiz.start(3, 76) &&
              quiz.settings.enabledNotes == 0,
          "unknown mask bits cannot enable a note and are removed when starting");

    quiz.settings.lowestMidi = 46;
    quiz.settings.highestMidi = 62;
    for (std::size_t i = 0; i < kNotes.size(); ++i) {
        quiz.settings.enabledNotes = std::uint32_t{1} << i;
        check(quiz.settings.allowedNoteCount() == 1 && quiz.start(2, 100 + static_cast<std::uint32_t>(i)),
              "every individual picker bit can start its own round");
        for (int question = 0; question < 2; ++question) {
            check(quiz.current() == &kNotes[i], "each picker bit maps to the exact note at its pool index");
            quiz.submit();
            quiz.next();
        }
    }
}

void positionMapping() {
    check(positionFromY(-100.0f, 700.0f) == 7, "mouse above canvas clamps to seventh");
    check(positionFromY(0.0f, 700.0f) == 7, "top edge is seventh position");
    check(positionFromY(99.0f, 700.0f) == 7, "top band includes its lower pixels");
    check(positionFromY(100.0f, 700.0f) == 6, "band boundary selects the next lower position");
    check(positionFromY(350.0f, 700.0f) == 4, "canvas midpoint is fourth position");
    check(positionFromY(699.0f, 700.0f) == 1, "bottom band is first position");
    check(positionFromY(700.0f, 700.0f) == 1, "bottom edge clamps to first");
    check(positionFromY(10000.0f, 700.0f) == 1, "mouse below canvas clamps to first");
    check(positionFromY(175.0f, 350.0f) == 4, "resize preserves relative position");
    constexpr std::array positions{7, 6, 5, 4, 3, 2, 1};
    Quiz drag;
    drag.start(1, 1);
    for (std::size_t band = 0; band < positions.size(); ++band) {
        const float center = static_cast<float>(band) * 100.0f + 50.0f;
        check(positionFromY(center, 700.0f) == positions[band],
              "all seven band centers select descending positions");
        check(positionFromY(center / 2.0f, 350.0f) == positions[band],
              "resizing preserves every band selection");
        check(drag.select(positionFromY(center, 700.0f)) &&
                  drag.selectedPosition() == positions[band],
              "dragging through every band changes the selected slide position");
        if (band > 0) {
            const float boundary = static_cast<float>(band) * 100.0f;
            check(positionFromY(boundary - 0.5f, 700.0f) == positions[band - 1] &&
                      positionFromY(boundary, 700.0f) == positions[band],
                  "each boundary changes to the next lower slide position");
        }
    }
    check(positionFromY(10.0f, 0.0f) == 1, "zero height is safe");
    check(positionFromY(10.0f, -1.0f) == 1, "negative height is safe");
    check(positionFromY(std::numeric_limits<float>::quiet_NaN(), 700.0f) == 1,
          "invalid pointer coordinate is safe");
    check(positionFromY(std::numeric_limits<float>::infinity(), 700.0f) == 1,
          "positive infinite pointer coordinate clamps to first");
    check(positionFromY(-std::numeric_limits<float>::infinity(), 700.0f) == 7,
          "negative infinite pointer coordinate clamps to seventh");
    check(positionFromY(10.0f, std::numeric_limits<float>::infinity()) == 1,
          "invalid canvas height is safe");
}
} // namespace

int main() {
    notePool();
    displayNames();
    roundTransitions();
    questionGeneration();
    noteRanges();
    normalizedRanges();
    exactPitchSelection();
    positionMapping();
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
