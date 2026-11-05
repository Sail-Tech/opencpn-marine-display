#include "md_wxpainter.h"

#include <algorithm>
#include <cmath>

#include <wx/brush.h>
#include <wx/font.h>
#include <wx/pen.h>

namespace {

wxColour Wx(md::Color c) { return wxColour(c.r, c.g, c.b, c.a); }
// Nautical angle (0 up, clockwise, degrees) to wxWidgets' (0 right, clockwise on screen, radians).
double Rad(double a) { return (a - 90.0) * md::kDeg; }

}  // namespace

void WxPainter::Fill(md::Color c) {
  gc_->SetPen(*wxTRANSPARENT_PEN);
  gc_->SetBrush(wxBrush(Wx(c)));
}

void WxPainter::Stroke(md::Color c, float width) {
  wxGraphicsPenInfo info(Wx(c), std::max(1.0, width * k_));
  info.Cap(wxCAP_ROUND).Join(wxJOIN_ROUND);
  gc_->SetPen(gc_->CreatePen(info));
  gc_->SetBrush(*wxTRANSPARENT_BRUSH);
}

void WxPainter::Rect(float x, float y, float w, float h, md::Color c, float radius) {
  Fill(c);
  if (radius > 0) gc_->DrawRoundedRectangle(X(x), Y(y), w * k_, h * k_, radius * k_);
  else gc_->DrawRectangle(X(x), Y(y), w * k_, h * k_);
}

void WxPainter::Frame(float x, float y, float w, float h, md::Color c, float width, float radius) {
  Stroke(c, width);
  if (radius > 0) gc_->DrawRoundedRectangle(X(x), Y(y), w * k_, h * k_, radius * k_);
  else gc_->DrawRectangle(X(x), Y(y), w * k_, h * k_);
}

void WxPainter::Disc(float cx, float cy, float r, md::Color c) {
  Fill(c);
  gc_->DrawEllipse(X(cx - r), Y(cy - r), 2 * r * k_, 2 * r * k_);
}

void WxPainter::Ring(float cx, float cy, float r, md::Color c, float width) {
  Stroke(c, width);
  gc_->DrawEllipse(X(cx - r), Y(cy - r), 2 * r * k_, 2 * r * k_);
}

void WxPainter::Band(float cx, float cy, float rIn, float rOut, float a0, float a1, md::Color c) {
  wxGraphicsPath path = gc_->CreatePath();
  path.AddArc(X(cx), Y(cy), rOut * k_, Rad(a0), Rad(a1), true);
  path.AddArc(X(cx), Y(cy), rIn * k_, Rad(a1), Rad(a0), false);
  path.CloseSubpath();
  Fill(c);
  gc_->FillPath(path);
}

void WxPainter::Line(float x1, float y1, float x2, float y2, md::Color c, float width) {
  Stroke(c, width);
  gc_->StrokeLine(X(x1), Y(y1), X(x2), Y(y2));
}

void WxPainter::Poly(const md::Pt* pts, int count, md::Color c, float stroke) {
  if (count < 2) return;
  wxGraphicsPath path = gc_->CreatePath();
  path.MoveToPoint(X(pts[0].x), Y(pts[0].y));
  for (int i = 1; i < count; ++i) path.AddLineToPoint(X(pts[i].x), Y(pts[i].y));
  path.CloseSubpath();
  if (stroke > 0) {
    Stroke(c, stroke);
    gc_->StrokePath(path);
  } else {
    Fill(c);
    gc_->FillPath(path);
  }
}

void WxPainter::Text(const std::string& text, float x, float y, float size, md::Color c, md::Align align,
                     bool bold) {
  const int px = std::max(6, static_cast<int>(std::lround(size * k_)));
  wxFontInfo info(wxSize(0, px));
  info.Family(wxFONTFAMILY_SWISS);
  if (bold) info.Bold();
  const wxFont font(info);
  gc_->SetFont(font, Wx(c));
  const wxString s = wxString::FromUTF8(text.c_str());
  double w = 0, h = 0, descent = 0, leading = 0;
  gc_->GetTextExtent(s, &w, &h, &descent, &leading);
  double left = X(x);
  if (align == md::Align::kCenter) left -= w / 2;
  if (align == md::Align::kRight) left -= w;
  // y is the baseline; DrawText takes the top of the text.
  gc_->DrawText(s, left, Y(y) - (h - descent));
}
