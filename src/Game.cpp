#include "Game.h"
#include "Trombone.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(int, consumeBrowserInputCancellation, (), {
    if (!Module['trainerInputHooked']) {
        const cancel = () => { Module['trainerInputCancelled'] = true; };
        Module['canvas'].addEventListener('blur', cancel);
        window.addEventListener('blur', cancel);
        document.addEventListener('visibilitychange', () => { if (document.hidden) cancel(); });
        Module['trainerInputHooked'] = true;
    }
    const cancelled = Module['trainerInputCancelled'];
    Module['trainerInputCancelled'] = false;
    return cancelled ? 1 : 0;
});
#else
static bool consumeBrowserInputCancellation() { return false; }
#endif

namespace trombone {
namespace {
const olc::Pixel background{11, 22, 38}, panel{19, 37, 58}, panelLight{27, 48, 71};
const olc::Pixel ink{241, 245, 246}, muted{153, 174, 194}, gold{238, 189, 100};
const olc::Pixel teal{93, 217, 195}, red{247, 154, 146}, line{42, 63, 84};
}

Game::Game() { sAppName = "Trombone Trainer"; }

bool Game::Rect::contains(olc::vf2d p) const {
    return p.x >= pos.x && p.y >= pos.y && p.x < pos.x + size.x && p.y < pos.y + size.y;
}

bool Game::OnUserCreate() {
    try { notation_ = std::make_unique<Notation>(); }
    catch (const std::exception& error) { notationError_ = error.what(); }
    return true;
}

bool Game::OnUserDestroy() {
    if (notation_) notation_->release(*this);
    return true;
}

std::uint32_t Game::seed() const {
    return static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
}

void Game::readPointer(float elapsed) {
    const bool focused = IsFocused();
    if (consumeBrowserInputCancellation() || (wasFocused_ && !focused)) {
        wasFocused_ = focused;
        drag_.cancel(); touchId_.reset();
        pointer_.pressed = pointer_.held = false; pointer_.released = true;
        mouseCooldown_ = 0.5f;
        return;
    }
    wasFocused_ = focused;
    pointer_.pressed = pointer_.released = false;
    mouseCooldown_ = std::max(0.0f, mouseCooldown_ - elapsed);
    const auto ids = GetTouch().GetTouchIDs();
    if (!ids.empty()) mouseCooldown_ = 0.5f;
    if (!touchId_) {
        for (auto id : ids) {
            const auto& t = GetTouch().GetTouch(id);
            if (t.bPressed) { touchId_ = id; pointer_.pressed = true; break; }
        }
    }
    if (touchId_) {
        mouseCooldown_ = 0.5f;
        if (GetTouch().IsTouch(*touchId_)) {
            const auto& t = GetTouch().GetTouch(*touchId_);
            pointer_.pos = t.position;
            pointer_.pressed |= t.bPressed;
            pointer_.held = t.bHeld;
            pointer_.released = t.bReleased;
        } else { pointer_.held = false; pointer_.released = true; }
        if (pointer_.released) touchId_.reset();
    } else if (mouseCooldown_ == 0) {
        const auto& mouse = GetMouse().GetButton(0);
        pointer_ = {GetMouse().GetPosition(), mouse.bPressed, mouse.bHeld, mouse.bReleased};
    } else { pointer_.held = false; }
    if (!IsFocused() && !touchId_) { pointer_.held = false; pointer_.pressed = false; }
}

void Game::text(std::string_view label, olc::vf2d pos, float size, olc::Pixel color) {
    const float fontScale = std::max(size * scale_, size <= 1.0f ? 1.0f : 1.4f);
    draw.String(pos, label, color, {fontScale, fontScale});
}

void Game::centered(std::string_view label, Rect box, float size, olc::Pixel color) {
    const float fit = std::min(std::max(1.4f, size * scale_), box.size.x / std::max(8.0f, float(label.size()) * 8.0f + 16.0f));
    const auto measure = draw.GetTextSize(label, false, {fit, fit});
    draw.String(box.pos + (box.size - measure) * 0.5f, label, color, {fit, fit});
}

void Game::wrapped(std::string_view label, Rect box, float size, olc::Pixel color) {
    const float fontScale = std::max(size * scale_, size <= 1.0f ? 1.0f : 1.4f);
    const auto columns = std::max(8, int(box.size.x / (8 * fontScale)));
    std::istringstream words{std::string(label)};
    std::string word, row;
    float y = box.pos.y;
    const float step = 12 * fontScale;
    auto flush = [&] {
        if (y + step > box.pos.y + box.size.y) return false;
        text(row, {box.pos.x, y}, size, color);
        row.clear(); y += step;
        return true;
    };
    while (words >> word) {
        if (!row.empty() && row.size() + word.size() + 1 > std::size_t(columns)) {
            if (!flush()) return;
        }
        while (word.size() > std::size_t(columns)) {
            row = word.substr(0, std::size_t(columns));
            word.erase(0, std::size_t(columns));
            if (!flush()) return;
        }
        if (!row.empty()) row += ' ';
        row += word;
    }
    if (!row.empty()) flush();
}

bool Game::button(std::string_view label, Rect box, bool active, bool enabled) {
    const bool hover = enabled && box.contains(pointer_.pos);
    draw.FilledRoundedRect(box.pos, box.size, 9 * scale_, active ? teal : hover ? panelLight : panel);
    draw.RoundedRect(box.pos, box.size, 9 * scale_, active ? teal : line);
    centered(label, box, 1.65f, enabled ? active ? background : ink : muted);
    return enabled && pointer_.pressed && hover;
}

void Game::drawClefs(Rect row) {
    const float gap = 10 * scale_, w = (row.size.x - gap) / 2;
    if (button("Bass", {row.pos, {w, row.size.y}}, quiz_.settings.clef == Clef::Bass))
        quiz_.settings.clef = Clef::Bass;
    if (button("Treble", {row.pos + olc::vf2d{w + gap, 0}, {w, row.size.y}}, quiz_.settings.clef == Clef::Treble))
        quiz_.settings.clef = Clef::Treble;
}

void Game::syncNotation() {
    const auto* note = quiz_.current();
    if (!note || !notation_) return;
    const bool treble = quiz_.settings.clef == Clef::Treble;
    if (renderedNotes_ == note->ldp && renderedTreble_ == treble) return;
    renderedNotes_ = note->ldp; renderedTreble_ = treble;
    try { notation_->render(*this, "(n " + std::string(note->ldp) + " q)", treble); notationError_.clear(); }
    catch (const std::exception& error) { notationError_ = error.what(); }
}

void Game::drawSettings() {
    if (notePicker_ && quiz_.settings.difficulty == Difficulty::Custom) {
        drawNotePicker(); return;
    }
    notePicker_ = false;
    const float x = margin_, w = contentWidth_ - 2 * x;
    const float gap = std::max(8.0f, 10 * scale_), control = std::max(44.0f, 48 * scale_);
    const bool wide = w >= 420 || height_ < 360;
    const bool compact = height_ < (wide ? 480 : 500);
    const bool showCustomCount = customCount_;
    auto heading = [&](std::string_view label, float y, float size, olc::Pixel color) {
        const float fit = std::min(size, w / draw.GetTextSize(label, true).x);
        draw.StringProp({x, y}, label, color, {fit, fit});
    };
    heading("Trombone Trainer", compact ? 4 : 18 * scale_, compact ? 2.0f : std::max(2.0f, 3.2f * scale_), gold);
    const float subtitleY = compact ? 25 : 18 * scale_ + 28 * scale_;
    heading("Powered by olcPixelGameEngine3", subtitleY, compact ? 1.25f : std::max(1.25f, 1.35f * scale_), muted);
    float y = compact ? 40 : std::max(height_ < 540 ? 62.0f : 100 * scale_, subtitleY + 25);
    if (!compact) { text("NOTES", {x, y}, 1.15f, muted); y += 20; }
    const int columns = wide ? 4 : 2;
    const float cell = (w - (columns - 1) * gap) / columns;
    const float rowGap = compact ? 4 : gap;
    const std::array difficulties{Difficulty::Easy, Difficulty::Medium, Difficulty::Hard, Difficulty::Custom};
    for (int i = 0; i < int(difficulties.size()); ++i) {
        const auto difficulty = difficulties[std::size_t(i)];
        if (button(difficultyName(difficulty), {{x + (i % columns) * (cell + gap), y + (i / columns) * (control + rowGap)}, {cell, control}},
                   quiz_.settings.difficulty == difficulty)) {
            quiz_.settings.difficulty = difficulty;
        }
    }
    const int rows = wide ? 1 : 2;
    y += rows * control + (rows - 1) * rowGap;
    if (!compact) {
        y += gap;
        if (quiz_.settings.difficulty == Difficulty::Custom) {
            const std::string label = "Choose notes (" + std::to_string(quiz_.settings.allowedNoteCount()) + ")";
            if (button(label, {{x, y}, {w, control}})) { notePicker_ = true; rangeTab_ = false; notePage_ = 0; }
            y += control;
        } else {
            const auto description = quiz_.settings.difficulty == Difficulty::Easy ? "Bb, C, D, Eb, F" :
                quiz_.settings.difficulty == Difficulty::Medium ? "Bb major (8 notes)" : "Bb Chromatic (13 notes)";
            centered(description, {{x, y}, {w, 20}}, 1.25f, muted);
            y += 20;
        }
    }
    const float startY = height_ - control - (compact ? 8 : 26);
    const float headingHeight = compact ? 0 : 20;
    const float counterHeight = showCustomCount ? control + rowGap : 0;
    const float sectionGap = std::clamp((startY - y - 2 * (control + headingHeight) - counterHeight) / 3, 4.0f, 36.0f * scale_);
    y += sectionGap;
    if (!compact) { text("CLEF", {x, y}, 1.15f, muted); y += 20; }
    drawClefs({{x, y}, {w, control}});
    y += control + sectionGap;
    if (!compact) { text("QUESTIONS", {x, y}, 1.15f, muted); y += 20; }
    const float countGap = wide ? gap : std::clamp((w - 3 * control - 76) / 3, 2.0f, gap);
    const float presetWidth = wide ? (w - 3 * countGap) / 4 : control;
    float countX = x;
    for (int count : {5, 10, 20}) {
        if (button(std::to_string(count), {{countX, y}, {presetWidth, control}}, !customCount_ && quiz_.settings.questionCount == count)) {
            quiz_.settings.questionCount = count; customCount_ = false;
        }
        countX += presetWidth + countGap;
    }
    if (button("Custom", {{countX, y}, {x + w - countX, control}}, customCount_)) customCount_ = true;
    if (showCustomCount) {
        y += control + rowGap;
        if (button("-", {{x, y}, {control, control}}, false, quiz_.settings.questionCount > 1)) --quiz_.settings.questionCount;
        centered(std::to_string(quiz_.settings.questionCount),
                 {{x + control + gap, y}, {w - 2 * (control + gap), control}}, 1.6f, gold);
        if (button("+", {{x + w - control, y}, {control, control}}, false, quiz_.settings.questionCount < 100)) ++quiz_.settings.questionCount;
    }
    float startX = x, startWidth = w;
    const bool compactCustom = compact && quiz_.settings.difficulty == Difficulty::Custom;
    if (compactCustom) {
        startWidth = (w - gap) / 2;
        if (button("Choose notes", {{x, startY}, {startWidth, control}})) {
            notePicker_ = true; rangeTab_ = false; notePage_ = 0;
        }
        startX += startWidth + gap;
    }
    const bool hasNotes = quiz_.settings.allowedNoteCount() > 0;
    if (button("Start!", {{startX, startY}, {startWidth, control}}, true, hasNotes)) {
        if (quiz_.start(quiz_.settings.questionCount, seed())) renderedNotes_.clear();
    }
}

void Game::drawCustomRange() {
    const float x = margin_, w = contentWidth_ - 2 * x;
    const float gap = 8, control = std::max(44.0f, 48 * scale_);
    const float top = height_ < 360 ? 84.0f : 104.0f;
    auto pitchLabel = [](int midi) {
        const auto note = std::find_if(kNotes.begin(), kNotes.end(),
                                      [midi](const Note& note) { return note.midi == midi; });
        return note->compactName();
    };
    auto adjust = [&](std::string_view label, int& setting, int minimum, int maximum, float y) {
        const float valueWidth = std::max(54.0f, w * 0.24f);
        const float minusX = x + w - 2 * control - valueWidth - 8;
        centered(label, {{x, y}, {minusX - x - 4, control}}, 1.4f, muted);
        if (button("-", {{minusX, y}, {control, control}}, false, setting > minimum)) --setting;
        centered(pitchLabel(setting), {{minusX + control + 4, y}, {valueWidth, control}}, 1.6f, gold);
        if (button("+", {{x + w - control, y}, {control, control}}, false, setting < maximum)) ++setting;
    };
    adjust("Low", quiz_.settings.lowestMidi, kNotes.back().midi, quiz_.settings.highestMidi, top);
    adjust("High", quiz_.settings.highestMidi, quiz_.settings.lowestMidi, kNotes.front().midi, top + control + gap);
    const float doneY = height_ - control - 16, previewY = top + 2 * (control + gap);
    const float previewHeight = std::min(180.0f * scale_, doneY - gap - previewY);
    if (previewHeight >= 70) {
        const auto low = std::find_if(kNotes.begin(), kNotes.end(),
            [&](const Note& note) { return note.midi == quiz_.settings.lowestMidi; });
        const auto high = std::find_if(kNotes.begin(), kNotes.end(),
            [&](const Note& note) { return note.midi == quiz_.settings.highestMidi; });
        std::string notes = "(n " + std::string(low->ldp) + " q)";
        if (low != high) notes += "(n " + std::string(high->ldp) + " q)";
        const bool treble = quiz_.settings.clef == Clef::Treble;
        if (notation_ && (notes != renderedNotes_ || treble != renderedTreble_)) {
            renderedNotes_ = notes; renderedTreble_ = treble;
            try { notation_->render(*this, notes, treble); notationError_.clear(); }
            catch (const std::exception& error) { notationError_ = error.what(); }
        }
        const Rect box{{x, previewY}, {w, previewHeight}};
        draw.FilledRoundedRect(box.pos, box.size, 8 * scale_, {250, 249, 244});
        if (notation_ && notationError_.empty())
            notation_->draw(*this, box.pos + olc::vf2d{6, 6}, box.size - olc::vf2d{12, 12});
        else wrapped("Notation unavailable: " + notationError_, box, 1.05f, background);
    }
    if (button("Done", {{x, doneY}, {w, control}}, true)) notePicker_ = false;
}

void Game::drawNotePicker() {
    const float x = margin_, w = contentWidth_ - 2 * x;
    const float gap = 8, control = std::max(44.0f, 48 * scale_);
    const bool shortView = height_ < 360;
    const float headingY = shortView ? 8.0f : 12.0f;
    const float summaryY = shortView ? 54.0f : 68.0f;
    const float filterY = shortView ? 72.0f : 92.0f;
    const float gridY = filterY + control + gap + 4;
    const float tabWidth = (w - 2 * gap) / 3;
    if (button("Back", {{x, headingY}, {tabWidth, 44}})) { notePicker_ = false; return; }
    if (button("Range", {{x + tabWidth + gap, headingY}, {tabWidth, 44}}, rangeTab_)) rangeTab_ = true;
    if (button("Notes", {{x + 2 * (tabWidth + gap), headingY}, {tabWidth, 44}}, !rangeTab_)) rangeTab_ = false;
    const auto low = std::find_if(kNotes.begin(), kNotes.end(),
        [&](const Note& note) { return note.midi == quiz_.settings.lowestMidi; });
    const auto high = std::find_if(kNotes.begin(), kNotes.end(),
        [&](const Note& note) { return note.midi == quiz_.settings.highestMidi; });
    const auto summary = std::string(low->compactName()) + " - " + std::string(high->compactName()) +
        " / " + std::to_string(quiz_.settings.allowedNoteCount()) + " included";
    centered(summary, {{x, summaryY}, {w, 16}}, 1.2f, muted);
    if (rangeTab_) { drawCustomRange(); return; }
    const float filterWidth = (w - gap) / 2;
    if (button("All", {{x, filterY}, {filterWidth, control}})) quiz_.settings.enabledNotes = kAllNotesMask;
    if (button("None", {{x + filterWidth + gap, filterY}, {filterWidth, control}})) quiz_.settings.enabledNotes = 0;

    const float startY = height_ - control - 16;
    const int columns = std::clamp(int((w + gap) / (64 + gap)), 2, 8);
    const int totalRows = (int(kNotes.size()) + columns - 1) / columns;
    const int rowsWithoutPaging = std::max(1, int((startY - gridY) / (control + gap)));
    const bool paged = totalRows > rowsWithoutPaging;
    const float navigationY = startY - gap - control;
    const float gridEnd = paged ? navigationY : startY;
    const int rows = std::max(1, int((gridEnd - gridY) / (control + gap)));
    const int capacity = columns * rows;
    const int pages = (int(kNotes.size()) + capacity - 1) / capacity;
    notePage_ = std::clamp(notePage_, 0, pages - 1);
    const float cell = (w - (columns - 1) * gap) / columns;
    const int first = notePage_ * capacity;
    for (int slot = first; slot < std::min(int(kNotes.size()), first + capacity); ++slot) {
        const auto index = kNotes.size() - 1 - std::size_t(slot);
        const auto& note = kNotes[index];
        const int local = slot - first;
        const Rect box{{x + (local % columns) * (cell + gap), gridY + (local / columns) * (control + gap)}, {cell, control}};
        const bool inRange = note.midi >= quiz_.settings.lowestMidi && note.midi <= quiz_.settings.highestMidi;
        if (button(note.compactName(), box, quiz_.settings.includes(note), inRange))
            quiz_.settings.enabledNotes ^= std::uint32_t{1} << index;
    }
    if (paged) {
        const float navWidth = (w - 2 * gap) / 3;
        if (button("Prev", {{x, navigationY}, {navWidth, control}}, false, notePage_ > 0)) --notePage_;
        centered(std::to_string(notePage_ + 1) + " / " + std::to_string(pages), {{x + navWidth + gap, navigationY}, {navWidth, control}}, 1.4f, muted);
        if (button("Next", {{x + 2 * (navWidth + gap), navigationY}, {navWidth, control}}, false, notePage_ + 1 < pages)) ++notePage_;
    } else {
        const float hintY = gridY + totalRows * (control + gap) + 8;
        if (startY - hintY > 52)
            wrapped("Teal notes are included. Gray notes are outside your range.", {{x, hintY}, {w, 52}}, 1.1f, muted);
    }
    if (button("Done", {{x, startY}, {w, control}}, true)) notePicker_ = false;
}

void Game::drawTrombone(Rect bounds) {
    drawInstrument(draw, bounds.pos, bounds.size, quiz_.selectedPosition());
}

void Game::drawQuestion(bool feedback) {
    const float x = margin_, w = contentWidth_ - 2 * x;
    const bool compact = height_ < 540;
    const float headerScale = std::max(1.0f, scale_);
    const float clefW = std::min(225.0f * headerScale, w * 0.49f);
    const Rect clefs{{contentWidth_ - x - clefW, 8 * headerScale}, {clefW, std::max(44.0f, 42 * scale_)}};
    const auto count = std::to_string(quiz_.questionCount());
    const float headerFont = std::min(std::max(1.4f, 1.35f * scale_),
        (clefs.pos.x - x - 8 * headerScale) / float(8 * (5 + 2 * count.size())));
    draw.String({x, 12 * headerScale}, "Q " + std::to_string(quiz_.questionIndex() + 1) + " / " + count,
                muted, {headerFont, headerFont});
    const auto score = "Score " + std::to_string(quiz_.score());
    draw.String({x, 31 * headerScale}, score, gold, {headerFont, headerFont});
    drawClefs(clefs);
    const auto* note = quiz_.current();
    if (!note) return;
    const Rect instrument{{x + 8 * scale_, height_ * (compact ? 0.51f : 0.52f)}, {w - 18 * scale_, height_ * 0.23f}};
    if (!feedback) {
        const auto geometry = InstrumentGeometry::make(instrument.pos.x, instrument.pos.y, instrument.size.x, instrument.size.y);
        if (pointer_.pressed) {
            drag_.cancel();
            if (pointer_.pos.x >= contentWidth_) drag_.beginColumn();
            else if (geometry.hitsSlide(pointer_.pos.x, pointer_.pos.y, quiz_.selectedPosition()))
                drag_.beginSlide(pointer_.pos.x, quiz_.selectedPosition(), geometry.stepPixels());
        }
        if (pointer_.held || pointer_.pressed || pointer_.released) {
            if (drag_.mode == SlideDrag::Mode::Column) quiz_.select(positionFromY(pointer_.pos.y, height_));
            else if (drag_.mode == SlideDrag::Mode::Slide) quiz_.select(drag_.updateSlide(pointer_.pos.x));
        }
        if (pointer_.released || (!pointer_.held && !pointer_.pressed)) drag_.cancel();
    }
    centered(note->displayName(), {{x, height_ * (compact ? 0.18f : 0.16f)}, {w, height_ * 0.085f}}, compact ? 3.5f : 4.5f, ink);
    const Rect paper{{x, height_ * (compact ? 0.29f : 0.27f)}, {w, height_ * (compact ? 0.20f : 0.23f)}};
    draw.FilledRoundedRect(paper.pos, paper.size, 10 * scale_, {250, 249, 244});
    syncNotation();
    if (notationError_.empty() && notation_) notation_->draw(*this, paper.pos + olc::vf2d{10, 7} * scale_, paper.size - olc::vf2d{20, 14} * scale_);
    else wrapped("Notation unavailable: " + notationError_, {paper.pos + olc::vf2d{14, 14} * scale_, paper.size - olc::vf2d{28, 28} * scale_}, 1.05f, background);
    drawTrombone(instrument);
    const Rect actionButton{{x, height_ * 0.84f}, {w, std::max(44.0f, 52 * scale_)}};
    if (!feedback) {
        constexpr std::string_view hint = "Move slide or choose position on the right";
        const float hintSize = w < float(hint.size()) * 8 * std::max(1.4f, 1.25f * scale_) ? 1.0f : 1.25f;
        wrapped(hint, {{x, height_ * (compact ? 0.74f : 0.76f)}, {w, 35 * scale_}}, hintSize, muted);
        if (button("Submit position " + std::to_string(quiz_.selectedPosition()), actionButton, true)) quiz_.submit();
    } else {
        const bool correct = quiz_.lastCorrect();
        text(correct ? "Correct!" : "Incorrect", {x, height_ * (compact ? 0.73f : 0.755f)}, 2.0f, correct ? teal : red);
        std::string answer = "Reference: " + std::to_string(note->referencePosition);
        std::string alternatives;
        for (int p = 1; p <= 7; ++p) if (p != note->referencePosition && note->accepts(p)) alternatives += (alternatives.empty() ? "" : ", ") + std::to_string(p);
        if (!alternatives.empty()) answer += " / Alt: " + alternatives;
        wrapped(answer, {{x, height_ * (compact ? 0.79f : 0.80f)}, {w, 35 * scale_}}, 1.25f, muted);
        if (button(quiz_.questionIndex() + 1 == quiz_.questionCount() ? "See results" : "Next question", actionButton, true)) quiz_.next();
    }
}

void Game::drawResults() {
    const float x = margin_, w = contentWidth_ - 2 * x;
    centered("PRACTICE COMPLETE", {{x, height_ * 0.15f}, {w, 45 * scale_}}, 2.2f, muted);
    centered(std::to_string(quiz_.score()) + " / " + std::to_string(quiz_.questionCount()), {{x, height_ * 0.29f}, {w, height_ * 0.13f}}, 6.0f, gold);
    const int percentage = int(std::lround(100.0 * quiz_.score() / std::max(1, quiz_.questionCount())));
    centered(std::to_string(percentage) + "% correct", {{x, height_ * 0.46f}, {w, 45 * scale_}}, 2.0f, teal);
    centered("Keep your slide moving.", {{x, height_ * 0.57f}, {w, 35 * scale_}}, 1.4f, muted);
    if (button("Play again", {{x, height_ * (height_ < 540 ? 0.65f : 0.70f)}, {w, std::max(44.0f, 56 * scale_)}}, true)) { quiz_.replay(seed()); renderedNotes_.clear(); }
    if (button("Settings", {{x, height_ * (height_ < 540 ? 0.80f : 0.81f)}, {w, std::max(44.0f, 48 * scale_)} })) quiz_.reset();
}

void Game::drawSelector() {
    const float segment = height_ / 7, side = width_ - contentWidth_;
    for (int row = 0; row < 7; ++row) {
        const int p = 7 - row;
        const bool selected = p == quiz_.selectedPosition();
        const Rect box{{contentWidth_, row * segment}, {side, segment}};
        draw.FilledRect(box.pos, box.size, selected ? teal : p % 2 ? panel : panelLight);
        draw.Line(box.pos, box.pos + olc::vf2d{side, 0}, line);
        centered(std::to_string(p), box, 3.3f, selected ? background : ink);
        if (selected) draw.FilledRect(box.pos, {5 * scale_, segment}, gold);
    }
}

bool Game::OnUserUpdate(float elapsed) {
    const auto physical = GetWindowSize();
    if (physical.x > 0 && physical.y > 0 && physical != ScreenSize()) {
        drag_.cancel();
        GetScreen().Resize(physical);
        draw.SetTarget(GetScreen());
    }
    width_ = float(ScreenSize().x); height_ = float(ScreenSize().y);
    const auto state = quiz_.state();
    const bool fullWidthSettings = state == QuizState::Settings;
    contentWidth_ = fullWidthSettings ? width_ : width_ * 0.78f;
    scale_ = std::clamp(std::min(width_ / 1120, height_ / 720), 0.7f, 2.0f);
    margin_ = std::min(42 * scale_, contentWidth_ * 0.07f);
    readPointer(elapsed);
    draw.Clear(background);
    switch (state) {
        case QuizState::Settings: drawSettings(); break;
        case QuizState::Question: drawQuestion(false); break;
        case QuizState::Feedback: drawQuestion(true); break;
        case QuizState::Results: drawResults(); break;
    }
    if (quiz_.state() != QuizState::Question) drag_.cancel();
    if (!fullWidthSettings) drawSelector();
    if (state != QuizState::Settings && height_ > width_) text("Rotate for a wider view", {margin_, height_ - 19 * scale_}, 1.0f, muted);
    return true;
}

} // namespace trombone
