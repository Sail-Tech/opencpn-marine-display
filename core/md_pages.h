#ifndef MD_PAGES_H_
#define MD_PAGES_H_

// The instrument pages, drawn in page coordinates (500 x 370) through the
// Painter, with the colours of a Palette.

#include "md_data.h"
#include "md_painter.h"
#include "md_theme.h"

namespace md {

enum class Page { kWindQuad = 0, kSteer, kAis, kGps, kDepth };
constexpr int kPageCount = 5;
const char* PageName(Page page);   // "Wind Quad — apparent and true wind"
const char* PageShort(Page page);  // "Wind Quad"
const char* PageKey(Page page);    // "windquad", for the settings
bool PageFromKey(const char* key, Page* page);

// What a pane remembers about the page it shows.
struct PageState {
  Page page = Page::kWindQuad;
  int ais_selected = 0;  // Index among the targets in range.
};

void DrawPage(Painter& p, const Palette& palette, const Data& data, PageState& state);

}  // namespace md

#endif  // MD_PAGES_H_
