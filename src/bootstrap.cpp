#define OLC_PGE3_APPLICATION
#include <olcPixelGameEngine3.h>
#include <iostream>
class Bootstrap : public olc::PixelGameEngine {
  bool OnUserCreate() override {
    std::cout << "PGE3 initialized (platform test only; no Lomse)" << std::endl;
    return true;
  }
  bool OnUserUpdate(float) override {
    draw.Clear(olc::Colour::DARK_BLUE);
    draw.String({24, 28}, "PGE3 PLATFORM TEST", olc::Colour::WHITE, {3,3});
    draw.String({24, 80}, "No notation in this diagnostic.", olc::Colour::WHITE, {2,2});
    return true;
  }
};
int main() {
  Bootstrap app;
  if (!app.Construct({960,540},{1,1})) return 1;
  return app.Start() ? 0 : 1;
}
