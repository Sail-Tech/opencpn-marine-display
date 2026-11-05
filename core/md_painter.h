#ifndef MD_PAINTER_H_
#define MD_PAINTER_H_

// The drawing interface the instrument pages use, after the ESP32 firmware's
// draw.h: a handful of primitives in page coordinates (500 x 370 for every
// page), mapped to the screen with one uniform scale so circles stay circles.
// The plugin implements it on wxGraphicsContext; the firmware on LVGL.
// Angles are nautical: 0 is up, clockwise in degrees.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace md {

constexpr float kPageWidth = 500.0f;
constexpr float kPageHeight = 370.0f;
constexpr double kDeg = 3.14159265358979323846 / 180.0;

struct Color {
  uint8_t r = 0, g = 0, b = 0, a = 255;
};

inline Color Rgb(uint32_t hex) {
  Color c;
  c.r = static_cast<uint8_t>((hex >> 16) & 0xff);
  c.g = static_cast<uint8_t>((hex >> 8) & 0xff);
  c.b = static_cast<uint8_t>(hex & 0xff);
  return c;
}

// "#1f4fb5" or "1f4fb5" to a colour; false for anything else.
inline bool ParseHex(const std::string& text, Color* out) {
  std::string s = text;
  while (!s.empty() && (s.front() == ' ' || s.front() == '#')) s.erase(0, 1);
  while (!s.empty() && s.back() == ' ') s.pop_back();
  if (s.size() != 6) return false;
  uint32_t v = 0;
  for (char ch : s) {
    int d;
    if (ch >= '0' && ch <= '9') d = ch - '0';
    else if (ch >= 'a' && ch <= 'f') d = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F') d = ch - 'A' + 10;
    else return false;
    v = v * 16 + static_cast<uint32_t>(d);
  }
  *out = Rgb(v);
  return true;
}

inline std::string ToHex(Color c) {
  char text[8];
  std::snprintf(text, sizeof(text), "#%02x%02x%02x", c.r, c.g, c.b);
  return text;
}

inline Color Mix(Color a, Color b, double t) {
  const auto m = [t](uint8_t x, uint8_t y) {
    return static_cast<uint8_t>(std::lround(x + (static_cast<double>(y) - x) * t));
  };
  Color c;
  c.r = m(a.r, b.r);
  c.g = m(a.g, b.g);
  c.b = m(a.b, b.b);
  c.a = m(a.a, b.a);
  return c;
}

inline Color Dim(Color c, double f) {
  const auto d = [f](uint8_t x) {
    return static_cast<uint8_t>(std::lround(std::fmax(0.0, std::fmin(255.0, x * f))));
  };
  return Color{d(c.r), d(c.g), d(c.b), c.a};
}

struct Pt {
  float x, y;
};

enum class Align { kLeft, kCenter, kRight };

class Painter {
 public:
  virtual ~Painter() = default;
  virtual void Rect(float x, float y, float w, float h, Color c, float radius = 0) = 0;
  virtual void Frame(float x, float y, float w, float h, Color c, float width, float radius = 0) = 0;
  virtual void Disc(float cx, float cy, float r, Color c) = 0;
  virtual void Ring(float cx, float cy, float r, Color c, float width) = 0;
  // The ring between rIn and rOut from nautical angle a0 to a1 (a0 < a1).
  virtual void Band(float cx, float cy, float rIn, float rOut, float a0, float a1, Color c) = 0;
  virtual void Line(float x1, float y1, float x2, float y2, Color c, float width) = 0;
  // A closed polygon, filled, or outlined when stroke > 0.
  virtual void Poly(const Pt* pts, int count, Color c, float stroke = 0) = 0;
  // UTF-8 text; y is the baseline; size is the font's pixel height in page units.
  virtual void Text(const std::string& text, float x, float y, float size, Color c,
                    Align align = Align::kLeft, bool bold = true) = 0;
};

// A point at nautical angle a (degrees) and distance r from (cx, cy).
inline float Px(float cx, double a, float r) { return cx + r * static_cast<float>(std::sin(a * kDeg)); }
inline float Py(float cy, double a, float r) { return cy - r * static_cast<float>(std::cos(a * kDeg)); }

}  // namespace md

#endif  // MD_PAINTER_H_
