#pragma once

#include <olcPixelGameEngine3.h>
#include <memory>
#include <string_view>

namespace trombone {

class Notation {
public:
  Notation();
  ~Notation();
  Notation(const Notation&) = delete;
  Notation& operator=(const Notation&) = delete;

  // ldpNotes is Lomse musicData content, e.g. "(n c4 q)(n d4 q)".
  void render(olc::PGEWindow& window, std::string_view ldpNotes, bool treble);
  void draw(olc::PGEWindow& window, olc::vf2d pos, olc::vf2d size);
  // Call from OnUserDestroy while the window's renderer still exists.
  void release(olc::PGEWindow& window);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace trombone
