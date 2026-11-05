#ifndef MD_WXPAINTER_H_
#define MD_WXPAINTER_H_

#include <wx/graphics.h>

#include "md_painter.h"

// The pages' Painter on a wxGraphicsContext: page coordinates (500 x 370)
// mapped to the window by screen = origin + page * scale.
class WxPainter : public md::Painter {
 public:
  WxPainter(wxGraphicsContext* gc, double ox, double oy, double k) : gc_(gc), ox_(ox), oy_(oy), k_(k) {}

  void Rect(float x, float y, float w, float h, md::Color c, float radius) override;
  void Frame(float x, float y, float w, float h, md::Color c, float width, float radius) override;
  void Disc(float cx, float cy, float r, md::Color c) override;
  void Ring(float cx, float cy, float r, md::Color c, float width) override;
  void Band(float cx, float cy, float rIn, float rOut, float a0, float a1, md::Color c) override;
  void Line(float x1, float y1, float x2, float y2, md::Color c, float width) override;
  void Poly(const md::Pt* pts, int count, md::Color c, float stroke) override;
  void Text(const std::string& text, float x, float y, float size, md::Color c, md::Align align,
            bool bold) override;

 private:
  double X(float x) const { return ox_ + x * k_; }
  double Y(float y) const { return oy_ + y * k_; }
  void Fill(md::Color c);
  void Stroke(md::Color c, float width);
  wxGraphicsContext* gc_;
  double ox_, oy_, k_;
};

#endif  // MD_WXPAINTER_H_
