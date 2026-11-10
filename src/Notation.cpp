#include "Notation.h"

#include <lomse_document.h>
#include <lomse_doorway.h>
#include <lomse_graphic_view.h>
#include <lomse_interactor.h>
#include <lomse_presenter.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace trombone {

struct Notation::Impl {
  static constexpr int width = 1200;
  static constexpr int height = 360;
  // Declaration order ensures that the presenter dies before its library.
  std::ostringstream diagnostics;
  lomse::LomseDoorway library{&diagnostics, &diagnostics};
  std::vector<unsigned char> pixels;
  std::unique_ptr<lomse::Presenter> presenter;
  olc::Image image;
  bool imageCreated = false;

  Impl() : pixels(width * height * 4, 255) {
    const std::string fonts = TRAINER_FONT_DIR;
    for (const auto* file : {"Bravura.otf", "LiberationSerif-Regular.ttf"}) {
      if (!std::ifstream(fonts + file, std::ios::binary))
        throw std::runtime_error("Notation font is missing: " + fonts + file);
    }
    library.init_library(lomse::k_pix_format_rgba32, 96, diagnostics);
    library.set_default_fonts_path(fonts);
  }
};

Notation::Notation() : impl_(std::make_unique<Impl>()) {}
Notation::~Notation() = default;

void Notation::render(olc::PGEWindow& window, std::string_view ldpNotes, bool treble) {
  auto& state = *impl_;
  state.diagnostics.str("");
  state.diagnostics.clear();
  const std::string source =
    "(lenmusdoc (vers 0.0)(content (score (vers 1.6)"
    "(instrument (musicData (clef " + std::string(treble ? "G" : "F4") +
    ")(key C)(time 4 4)" + std::string(ldpNotes) + ")))))";
  auto presenter = std::unique_ptr<lomse::Presenter>(state.library.new_document(
    lomse::k_view_single_system, source, lomse::Document::k_format_ldp,
    state.diagnostics));
  if (!presenter)
    throw std::runtime_error("Lomse could not create the notation document.");
  auto document = presenter->get_document();
  document.set_page_left_margin_odd(200);
  document.set_page_right_margin_odd(200);
  document.set_page_top_margin_odd(200);
  document.set_page_bottom_margin_odd(200);
  document.set_page_left_margin_even(200);
  document.set_page_right_margin_even(200);
  document.set_page_top_margin_even(200);
  document.set_page_bottom_margin_even(200);
  document.end_of_changes();
  auto interactor = presenter->get_interactor(0).lock();
  if (!interactor)
    throw std::runtime_error("Lomse could not create the notation view.");

  std::fill(state.pixels.begin(), state.pixels.end(), 255);
  interactor->set_rendering_buffer(state.pixels.data(), Impl::width, Impl::height);
  interactor->set_view_background(lomse::Color(255, 255, 255));
  const auto page = interactor->get_page_size();
  if (page.width <= 0 || page.height <= 0)
    throw std::runtime_error("Lomse produced an invalid notation page.");
  // Logical units are hundredths of a millimetre. Compute the fit explicitly:
  // Lomse 0.30.0 zoom_fit_full incorrectly uses screen width for both axes.
  const double pixelsPerLogicalUnit = state.library.get_screen_ppi() / 2540.0;
  const double scale = 0.94 * std::min(
    Impl::width / (double(page.width) * pixelsPerLogicalUnit),
    Impl::height / (double(page.height) * pixelsPerLogicalUnit));
  interactor->set_scale(scale, 0, 0, false);
  interactor->new_viewport(0, 0, false);
  interactor->redraw_bitmap();

  // Crop Lomse's page margins, then preserve the engraved aspect ratio on draw.
  int left = Impl::width, top = Impl::height, right = -1, bottom = -1;
  for (int y = 0; y < Impl::height; ++y) {
    for (int x = 0; x < Impl::width; ++x) {
      const auto index = std::size_t(y * Impl::width + x) * 4;
      if (state.pixels[index] < 224 || state.pixels[index + 1] < 224 ||
          state.pixels[index + 2] < 224) {
        left = std::min(left, x);
        top = std::min(top, y);
        right = std::max(right, x);
        bottom = std::max(bottom, y);
      }
    }
  }
  if (right < left || bottom < top)
    throw std::runtime_error("Lomse produced an empty notation bitmap. " +
                             state.diagnostics.str());
  constexpr int padding = 20;
  left = std::max(0, left - padding);
  top = std::max(0, top - padding);
  right = std::min(Impl::width - 1, right + padding);
  bottom = std::min(Impl::height - 1, bottom + padding);
  const olc::vi2d cropSize{right - left + 1, bottom - top + 1};
  if (!state.imageCreated) {
    olc::ImageConfig config;
    config.Filtered = true;
    if (!window.CreateImage(state.image, cropSize, config))
      throw std::runtime_error("PGE3 could not create the notation image.");
    state.imageCreated = true;
  } else {
    state.image.Resize(cropSize);
  }
  static_assert(sizeof(olc::Pixel) == 4, "PGE3 pixels must match Lomse RGBA bytes");
  auto* destination = reinterpret_cast<unsigned char*>(state.image.Data());
  for (int row = 0; row < cropSize.y; ++row) {
    const auto* sourceRow = state.pixels.data() +
      std::size_t((top + row) * Impl::width + left) * 4;
    std::memcpy(destination + std::size_t(row * cropSize.x) * 4,
                sourceRow, std::size_t(cropSize.x) * 4);
  }
  state.image.BindCPU();
  state.presenter = std::move(presenter);
}

void Notation::draw(olc::PGEWindow& window, olc::vf2d pos, olc::vf2d size) {
  auto& state = *impl_;
  if (!state.imageCreated || size.x <= 0 || size.y <= 0)
    return;
  const auto imageSize = state.image.Size();
  const float scale = std::min(size.x / float(imageSize.x), size.y / float(imageSize.y));
  const olc::vf2d drawnSize{float(imageSize.x) * scale, float(imageSize.y) * scale};
  window.GetDraw().ImageRect(state.image.all(), pos + (size - drawnSize) * 0.5f, drawnSize);
}

void Notation::release(olc::PGEWindow& window) {
  if (impl_->imageCreated) {
    window.DestroyImage(impl_->image);
    impl_->imageCreated = false;
  }
  impl_->presenter.reset();
}

} // namespace trombone
