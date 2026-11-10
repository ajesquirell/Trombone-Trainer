#pragma once

#include "olcPixelGameEngine3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace trombone {

// Painter is olc::Draw in the game; the same primitives can be recorded offline.
template<class Painter>
void drawInstrument(Painter& draw, olc::vf2d pos, olc::vf2d size, int position) {
    if (size.x <= 0 || size.y <= 0) return;
    const float scale = std::min(size.x / 1000.0f, size.y / 180.0f);
    const auto origin = pos + (size - olc::vf2d{1000, 180} * scale) * 0.5f;
    const auto point = [&](olc::vf2d p) { return origin + p * scale; };
    const olc::Pixel outline{76, 51, 26}, brass{199, 148, 56}, light{255, 225, 146};
    const olc::Pixel silver{180, 194, 204}, silverLight{239, 246, 248};
    constexpr float pi = 3.14159265f;
    auto batch = draw.CreateFilledBatch();
    const auto triangle = [&](olc::vf2d a, olc::vf2d b, olc::vf2d c, olc::Pixel color) {
        draw.FilledTriangle(batch, point(a), point(b), point(c), color);
    };
    const auto quad = [&](olc::vf2d a, olc::vf2d b, olc::vf2d c, olc::vf2d d, olc::Pixel color) {
        triangle(a, b, c, color); triangle(a, c, d, color);
    };
    const auto ellipse = [&](olc::vf2d center, float rx, float ry, olc::Pixel color) {
        for (int i = 0; i < 48; ++i) {
            const float a = 2 * pi * i / 48, b = 2 * pi * (i + 1) / 48;
            triangle(center, center + olc::vf2d{rx * std::cos(a), ry * std::sin(a)},
                     center + olc::vf2d{rx * std::cos(b), ry * std::sin(b)}, color);
        }
    };
    const auto stroke = [&](const std::vector<olc::vf2d>& path, float width,
                            olc::Pixel color, olc::vf2d offset = {}) {
        for (std::size_t i = 1; i < path.size(); ++i) {
            const auto normal = [&](std::size_t n) {
                auto t = path[std::min(n + 1, path.size() - 1)] - path[n == 0 ? 0 : n - 1];
                const float length = std::sqrt(t.x * t.x + t.y * t.y);
                return olc::vf2d{-t.y, t.x} * (width * 0.5f / length);
            };
            const auto a = path[i - 1] + offset, b = path[i] + offset;
            quad(a + normal(i - 1), b + normal(i), b - normal(i), a - normal(i - 1), color);
        }
        for (const auto p : {path.front(), path.back()})
            draw.FilledCircle(batch, point(p + offset), width * scale * 0.5f, color);
    };
    const auto tube = [&](const std::vector<olc::vf2d>& path, float width = 12,
                          bool chrome = false) {
        stroke(path, width + 3, outline);
        stroke(path, width, chrome ? silver : brass);
        stroke(path, width * 0.20f, chrome ? silverLight : light, {0, -width * 0.22f});
    };
    const auto arc = [&](olc::vf2d center, float radius, bool right) {
        std::vector<olc::vf2d> path;
        for (int i = 0; i <= 24; ++i) {
            const float a = -pi / 2 + pi * i / 24;
            path.push_back(center + olc::vf2d{(right ? 1.0f : -1.0f) * radius * std::cos(a),
                                             radius * std::sin(a)});
        }
        return path;
    };

    // Fixed bell section: parallel branches joined by the rear tuning bow.
    tube({{60, 88}, {280, 88}});
    tube(arc({60, 116}, 28, false));
    tube({{60, 144}, {220, 144}});
    tube({{106, 88}, {106, 144}}, 5);
    tube({{235, 88}, {235, 96}}, 5);

    // Curved bell profile, shaded across its surface instead of a triangular cone.
    constexpr std::array<float, 7> bands{-1, -0.74f, -0.3f, 0.04f, 0.44f, 0.82f, 1};
    const std::array<olc::Pixel, 6> colors{{{151, 99, 33}, {243, 193, 92}, {255, 226, 150},
                                         {226, 173, 69}, {188, 126, 37}, {120, 75, 26}}};
    const auto bell = [](float t, float band) {
        return olc::vf2d{270 + 260 * t, 88 + band * (6 + 68 * std::pow(t, 3.6f))};
    };
    for (int i = 0; i < 36; ++i) {
        const float a = i / 36.0f, b = (i + 1) / 36.0f;
        for (std::size_t j = 0; j + 1 < bands.size(); ++j)
            quad(bell(a, bands[j]), bell(b, bands[j]), bell(b, bands[j + 1]), bell(a, bands[j + 1]), colors[j]);
    }
    for (const float edge : {-1.0f, 1.0f}) {
        std::vector<olc::vf2d> contour;
        for (int i = 0; i <= 36; ++i) contour.push_back(bell(i / 36.0f, edge));
        stroke(contour, 2.5f, outline);
    }
    ellipse({530, 88}, 14, 75, outline);
    ellipse({531, 88}, 12, 72, light);
    ellipse({533, 88}, 9, 66, {92, 55, 24});
    ellipse({535, 88}, 6, 61, {44, 32, 23});

    // Mouthpiece and the two stationary nickel-silver inner slide rails.
    tube({{200, 96}, {635, 96}}, 8, true);
    tube({{220, 144}, {635, 144}}, 8, true);
    tube({{235, 96}, {235, 144}}, 5);
    quad({177, 84}, {190, 88}, {205, 93}, {205, 99}, silver);
    quad({177, 84}, {205, 99}, {190, 104}, {177, 108}, silver);
    tube({{202, 96}, {220, 96}}, 7, true);
    ellipse({177, 96}, 3, 12, silverLight);
    ellipse({176, 96}, 1.5f, 8, {85, 99, 110});

    // A rigid outer slide translates; its two sleeves always overlap the inner rails.
    const float travel = 50.0f * (std::clamp(position, 1, 7) - 1);
    const float start = 250 + travel, end = 650 + travel;
    tube({{start, 96}, {end, 96}});
    tube({{start, 144}, {end, 144}});
    tube(arc({end, 120}, 24, true));
    tube({{start + 24, 96}, {start + 24, 144}}, 5);
    for (const float y : {96.0f, 144.0f}) tube({{start, y}, {start + 12, y}}, 14, true);
    // Small water key stays attached to the moving end bow.
    tube({{end - 9, 147}, {end + 10, 153}}, 3, true);
    ellipse({end + 11, 152}, 4, 2.5f, brass);
    draw.Batch(batch);
}

} // namespace trombone
