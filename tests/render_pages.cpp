// Renders every page with demo data to SVG files, for looking at the layouts
// without OpenCPN:
//   c++ -std=c++17 -Icore tests/render_pages.cpp core/*.cpp -o render_pages
//   ./render_pages out_dir [day|night]
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "md_demo.h"
#include "md_pages.h"
#include "md_theme.h"

using namespace md;

namespace {

std::string Css(Color c) {
  std::ostringstream s;
  s << "rgba(" << int(c.r) << ',' << int(c.g) << ',' << int(c.b) << ',' << c.a / 255.0 << ')';
  return s.str();
}

std::string Escape(const std::string& t) {
  std::string out;
  for (char ch : t) {
    if (ch == '<') out += "&lt;";
    else if (ch == '>') out += "&gt;";
    else if (ch == '&') out += "&amp;";
    else out += ch;
  }
  return out;
}

class SvgPainter : public Painter {
 public:
  std::ostringstream out;
  void Rect(float x, float y, float w, float h, Color c, float r) override {
    out << "<rect x='" << x << "' y='" << y << "' width='" << w << "' height='" << h << "' rx='" << r
        << "' fill='" << Css(c) << "'/>\n";
  }
  void Frame(float x, float y, float w, float h, Color c, float width, float r) override {
    out << "<rect x='" << x << "' y='" << y << "' width='" << w << "' height='" << h << "' rx='" << r
        << "' fill='none' stroke='" << Css(c) << "' stroke-width='" << width << "'/>\n";
  }
  void Disc(float cx, float cy, float r, Color c) override {
    out << "<circle cx='" << cx << "' cy='" << cy << "' r='" << r << "' fill='" << Css(c) << "'/>\n";
  }
  void Ring(float cx, float cy, float r, Color c, float width) override {
    out << "<circle cx='" << cx << "' cy='" << cy << "' r='" << r << "' fill='none' stroke='" << Css(c)
        << "' stroke-width='" << width << "'/>\n";
  }
  void Band(float cx, float cy, float rIn, float rOut, float a0, float a1, Color c) override {
    const int large = a1 - a0 > 180 ? 1 : 0;
    out << "<path d='M" << Px(cx, a0, rOut) << ',' << Py(cy, a0, rOut) << " A" << rOut << ',' << rOut << " 0 " << large
        << " 1 " << Px(cx, a1, rOut) << ',' << Py(cy, a1, rOut) << " L" << Px(cx, a1, rIn) << ',' << Py(cy, a1, rIn)
        << " A" << rIn << ',' << rIn << " 0 " << large << " 0 " << Px(cx, a0, rIn) << ',' << Py(cy, a0, rIn)
        << " Z' fill='" << Css(c) << "'/>\n";
  }
  void Line(float x1, float y1, float x2, float y2, Color c, float width) override {
    out << "<line x1='" << x1 << "' y1='" << y1 << "' x2='" << x2 << "' y2='" << y2 << "' stroke='" << Css(c)
        << "' stroke-width='" << width << "' stroke-linecap='round'/>\n";
  }
  void Poly(const Pt* p, int n, Color c, float stroke) override {
    out << "<polygon points='";
    for (int i = 0; i < n; ++i) out << p[i].x << ',' << p[i].y << ' ';
    if (stroke > 0) out << "' fill='none' stroke='" << Css(c) << "' stroke-width='" << stroke << "' stroke-linejoin='round'/>\n";
    else out << "' fill='" << Css(c) << "'/>\n";
  }
  void Text(const std::string& t, float x, float y, float size, Color c, Align a, bool bold) override {
    out << "<text x='" << x << "' y='" << y << "' font-family='Helvetica, Arial, sans-serif' font-size='" << size
        << "' font-weight='" << (bold ? "bold" : "normal") << "' fill='" << Css(c) << "' text-anchor='"
        << (a == Align::kLeft ? "start" : a == Align::kCenter ? "middle" : "end") << "'>" << Escape(t) << "</text>\n";
  }
};

}  // namespace

int main(int argc, char** argv) {
  const std::string dir = argc > 1 ? argv[1] : ".";
  const bool night = argc > 2 && std::string(argv[2]) == "night";
  Demo demo;
  demo.Step(1);
  Data data;
  demo.Fill(&data);
  const Palette palette = MakePalette(DefaultTheme(), night ? Mode::kNight : Mode::kDay, Scheme::kDay);
  for (int i = 0; i < kPageCount; ++i) {
    SvgPainter painter;
    PageState state;
    state.page = static_cast<Page>(i);
    DrawPage(painter, palette, data, state);
    const std::string path = dir + "/" + std::to_string(i < 10 ? 0 : 1) + std::to_string(i % 10) + "-" + PageKey(state.page) + ".svg";
    std::ofstream file(path);
    file << "<svg xmlns='http://www.w3.org/2000/svg' width='500' height='370' viewBox='0 0 500 370'>\n"
         << "<clipPath id='page'><rect width='500' height='370'/></clipPath><g clip-path='url(#page)'>\n"
         << painter.out.str() << "</g></svg>\n";
    std::cout << path << '\n';
  }
  return 0;
}
