#define OLC_PGE3_APPLICATION
#include <olcPixelGameEngine3.h>
#include "Notation.h"
#include <array>
#include <iostream>
#include <memory>

class Compatibility : public olc::PixelGameEngine {
  std::unique_ptr<trombone::Notation> notation;
  bool treble = false;
  int pitch = 0;
  std::string error;
  bool dirty = true;
  static constexpr std::array<const char*,4> pitches{"d4", "+c4", "-b3", "c3"};
  static constexpr std::array<const char*,4> names{"D4", "C-sharp 4", "B-flat 3", "C3 (ledger lines)"};

  bool OnUserCreate() override {
    try { notation = std::make_unique<trombone::Notation>(); }
    catch (const std::exception& e) { error=e.what(); std::cerr << error << '\n'; }
    return true;
  }
  bool OnUserDestroy() override {
    if (notation) notation->release(*this);
    notation.reset();
    return true;
  }
  bool OnUserUpdate(float) override {
    const auto window = GetWindowSize();
    if (window.x > 0 && window.y > 0 && window != ScreenSize()) GetScreen().Resize(window);
    const float w = float(ScreenSize().x), h = float(ScreenSize().y);
    olc::vf2d pointer = GetMouse().GetPosition();
    bool pressed = GetMouse().GetButton(0).bPressed;
    const auto ids=GetTouch().GetTouchIDs();
    if (!ids.empty()) {
      pressed=false;
      for (auto id:ids) {
        const auto& t=GetTouch().GetTouch(id);
        if(t.bPressed) { pointer=t.position; pressed=true; break; }
      }
    }
    if (pressed && pointer.y >= h*.76f && pointer.y < h*.93f) {
      if(pointer.x < w*.5f) treble=!treble;
      else pitch=(pitch+1)%int(pitches.size());
      dirty=true;
    }
    if(dirty && notation) {
      try {
        notation->render(*this, std::string("(n ")+pitches[pitch]+" q)(n e4 q)(n g4 q)", treble);
        error.clear();
        std::cout << "Lomse rendered " << names[pitch] << (treble?" treble":" bass") << std::endl;
      } catch (const std::exception& e) { error=e.what(); std::cerr<<error<<'\n'; }
      dirty=false;
    }
    draw.Clear(olc::Pixel(16,29,42));
    const float scale=std::max(.8f,std::min(w/430.f,h/200.f));
    draw.String({w*.04f,h*.04f},"PGE3 + LOMSE",olc::Colour::WHITE,{scale,scale});
    draw.String({w*.04f,h*.16f},std::string(names[pitch])+(treble?" / Treble":" / Bass"),olc::Colour::WHITE,{scale*.6f,scale*.6f});
    if (notation && error.empty()) notation->draw(*this,{w*.04f,h*.28f},{w*.92f,h*.40f});
    if (!error.empty()) draw.String({w*.04f,h*.30f},"Notation failed: "+error,olc::Colour::RED);
    draw.FilledRect({w*.04f,h*.76f},{w*.43f,h*.17f},olc::Pixel(32,94,102));
    draw.FilledRect({w*.53f,h*.76f},{w*.43f,h*.17f},olc::Pixel(32,94,102));
    draw.String({w*.07f,h*.82f},"Switch clef",olc::Colour::WHITE,{scale*.6f,scale*.6f});
    draw.String({w*.56f,h*.82f},"Next pitch",olc::Colour::WHITE,{scale*.6f,scale*.6f});
    return true;
  }
};
int main() {
  Compatibility app;
  olc::PGEConfig cfg;
  cfg.vScreenSize={1100,650}; cfg.vPixelSize={1,1};
  cfg.bRetainAspectRatio=false; cfg.sAppName="PGE3 + Lomse compatibility";
  if(!app.Construct(cfg)) return 1;
  return app.Start()?0:1;
}
