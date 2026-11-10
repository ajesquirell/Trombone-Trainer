#include "Trombone.h"
#include <fstream>
#include <string>

struct Recorder {
    std::ofstream out;
    bool first = true;
    Recorder(const char* path) : out(path) { out << '['; }
    ~Recorder() { out << ']'; }
    int CreateFilledBatch() { return 0; }
    void Batch(int&) {}
    void item(const char* kind, olc::Pixel color) {
        if (!first) out << ',';
        first = false;
        out << "{\"kind\":\"" << kind << "\",\"color\":[" << int(color.r) << ',' << int(color.g) << ',' << int(color.b) << "],\"p\":[";
    }
    void FilledTriangle(int&, olc::vf2d a, olc::vf2d b, olc::vf2d c, olc::Pixel color) {
        item("triangle", color);
        out << a.x << ',' << a.y << ',' << b.x << ',' << b.y << ',' << c.x << ',' << c.y << "]}";
    }
    void FilledCircle(int&, olc::vf2d p, float radius, olc::Pixel color) {
        item("circle", color);
        out << p.x << ',' << p.y << ',' << radius << "]}";
    }
};
int main(int argc, char** argv) {
    if (argc != 5) return 1;
    Recorder draw(argv[1]);
    const float width = std::stof(argv[3]), height = std::stof(argv[4]);
    trombone::drawInstrument(draw, {10, 20}, {width - 20, height - 40}, std::stoi(argv[2]));
}
