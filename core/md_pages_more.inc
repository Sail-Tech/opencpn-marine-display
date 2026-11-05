// The Theme Studio's further instrument views, in its page coordinates (500 x 370).
// Included by md_pages.cpp inside its unnamed namespace, so they share its helpers.

// ---------- Shared helpers ----------

Color Alpha(Color c, double a) {
  c.a = static_cast<uint8_t>(std::lround(std::fmax(0.0, std::fmin(1.0, a)) * 255));
  return c;
}

// Three digits, "005".
std::string Three(double deg) {
  char text[16];
  std::snprintf(text, sizeof(text), "%03d", static_cast<int>(std::lround(std::fmod(std::fmod(deg, 360) + 360, 360))) % 360);
  return text;
}

// A true direction as shown: magnetic when the variation is known.
double Shown(const Data& d, double true_deg) { return d.variation_ok ? true_deg - d.variation : true_deg; }
const char* Ref(const Data& d) { return d.variation_ok ? "\xc2\xb0M" : "\xc2\xb0T"; }

// XTE with the side of the track the boat is on.
std::string Xte(double xte, int decimals) {
  return Fmt(decimals == 1 ? "%.1f" : "%.2f", std::fabs(xte)) + (xte < 0 ? "L" : xte > 0 ? "R" : "");
}

void Polyline(Painter& p, const std::vector<Pt>& pts, Color c, float width) {
  for (size_t i = 1; i < pts.size(); ++i) p.Line(pts[i - 1].x, pts[i - 1].y, pts[i].x, pts[i].y, c, width);
}

void Dashed(Painter& p, float x, float y0, float y1, Color c, float width) {
  for (float y = y0; y < y1; y += 12) p.Line(x, y, x, std::fmin(y + 7, y1), c, width);
}

// The smallest of the steps that holds |v| with a margin.
double Scale(double v, std::initializer_list<double> steps) {
  double last = 1;
  for (double s : steps) {
    last = s;
    if (std::fabs(v) <= s * 0.8) return s;
  }
  return last;
}

// x of a history sample: the newest at the right edge, `span` samples across.
float AgeX(float x, float w, size_t i, size_t n, size_t span) {
  return x + w - w * static_cast<float>(n - 1 - i) / static_cast<float>(span - 1);
}

// The filled history graph of the Barometer and SOG pages.
void AreaGraph(Painter& p, const Palette& pl, const std::vector<float>& v, size_t span, float x, float y, float w,
               float h, double min, double max, float line) {
  if (v.size() < 2) return;
  const size_t first = v.size() > span ? v.size() - span : 0;
  std::vector<Pt> pts;
  for (size_t i = first; i < v.size(); ++i) {
    const double f = (std::fmax(min, std::fmin(max, v[i])) - min) / (max - min);
    pts.push_back({AgeX(x, w, i - first, v.size() - first, span), y + h - static_cast<float>(f) * h});
  }
  std::vector<Pt> area = pts;
  area.push_back({pts.back().x, y + h});
  area.push_back({pts.front().x, y + h});
  p.Poly(area.data(), static_cast<int>(area.size()), Alpha(pl.c[ARROWS], 0.8));
  Polyline(p, pts, pl.c[ARROWS], line);
}

// Pressure range: 990 to 1020 mb, widened in steps of 10 to hold the samples.
void PressureRange(const Data& d, double* min, double* max) {
  *min = 990;
  *max = 1020;
  for (float v : d.pressure_history) {
    while (v < *min) *min -= 10;
    while (v > *max) *max += 10;
  }
  if (d.pressure_ok) {
    while (d.pressure < *min) *min -= 10;
    while (d.pressure > *max) *max += 10;
  }
}

std::string Clock(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0 || seconds >= 100 * 3600) return "--:--:--";
  const int s = static_cast<int>(std::lround(seconds));
  char text[32];
  std::snprintf(text, sizeof(text), "%02d:%02d:%02d", s / 3600, (s / 60) % 60, s % 60);
  return text;
}

// ---------- Highway ----------

void Highway(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const bool w = d.waypoint_ok;
  const double eta = d.position_ok && d.sog > 0.1 ? d.dtw / d.sog * 3600 : NAN;
  p.Text("ETW", 16, 34, 22, c[PRIMARY_TEXT]);
  p.Text(w ? Clock(eta) : "--:--:--", 250, 54, 52, c[PRIMARY_TEXT], Align::kCenter);
  std::string name = w ? d.waypoint : "";
  if (name.size() > 10) name.resize(10);
  p.Text(name, 480, 34, 20, c[SECONDARY_TEXT], Align::kRight);

  // The track moves sideways by the XTE; the boat stays in the middle.
  const double scale = Scale(w ? d.xte : 0, {0.1, 0.2, 0.5, 1, 2, 5});
  const float bottom = w ? static_cast<float>(-std::fmax(-1.0, std::fmin(1.0, d.xte / scale)) * 110) : 0;
  const float top = bottom * 0.32f;
  const Color sea = Mix(c[DISPLAY_BG], c[GRID], 0.35), lane = Mix(c[DISPLAY_BG], c[SECONDARY_TEXT], 0.4);
  const Color lines = Mix(c[SECONDARY_TEXT], c[DISPLAY_BG], 0.15);
  const Pt outer[4] = {{51, 76}, {449, 76}, {500, 247}, {0, 247}};
  p.Poly(outer, 4, sea);
  const Pt road[4] = {{199 + top, 76}, {301 + top, 76}, {408 + bottom, 247}, {92 + bottom, 247}};
  p.Poly(road, 4, lane);
  for (int i = 0; i < 7; ++i) {
    const float y = 74 + static_cast<float>(std::pow(i / 6.0, 1.65)) * 170;
    p.Line(0, y, 500, y, lines, 1.5f);
  }
  for (int i = -3; i <= 3; ++i) {
    // The outer lines leave the screen at its sides: end them there.
    float x0 = 250 + top + i * 32.0f, y0 = 76, x1 = 250 + bottom + i * 110.0f, y1 = 247;
    for (const float edge : {0.0f, kPageWidth}) {
      if ((x1 - edge) * (x0 - edge) < 0) {
        const float t = (edge - x0) / (x1 - x0);
        if ((edge == 0 && x1 < 0) || (edge > 0 && x1 > edge)) {
          y1 = y0 + (y1 - y0) * t;
          x1 = edge;
        }
      }
    }
    p.Line(x0, y0, x1, y1, lines, 2);
  }
  p.Line(250 + top, 75, 250 + bottom, 247, c[PORT], 8);
  if (w) p.Disc(250 + top + (bottom - top) * (114 - 76) / 171.0f, 114, 13, c[WARNING]);
  Hull(p, pl, 250, 223, 46);

  p.Line(0, 250, 500, 250, c[SEPARATOR], 1);
  p.Line(170, 254, 170, 363, c[SEPARATOR], 1);
  p.Line(330, 254, 330, 363, c[SEPARATOR], 1);
  p.Text(std::string("WPT BRG ") + Ref(d), 10, 276, 17, c[PRIMARY_TEXT]);
  p.Text("XTE", 181, 276, 18, c[PRIMARY_TEXT]);
  p.Text("WPT DIST NM", 340, 276, 16, c[PRIMARY_TEXT]);
  p.Text(w ? Three(Shown(d, d.btw)) : "---", 85, 355, 70, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(w ? Xte(d.xte, std::fabs(d.xte) < 1 ? 2 : 1) : "-.--", 249, 355, 58, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(w ? Fmt(d.dtw < 10 ? "%.2f" : "%.1f", d.dtw) : "-.--", 418, 355, 64, c[PRIMARY_TEXT], Align::kCenter);
  if (!w) Unavailable(p, pl, "No active waypoint");
}

// ---------- Laylines ----------

void Laylines(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const float cx = 250, cy = 197, k = 0.7088f;
  const double hdg = d.heading_ok ? d.hdg : 0;
  Rose(p, pl, cx, cy, 152, false);
  // The compass ring turns with the heading.
  p.Disc(cx, cy, 99, c[SECONDARY_TEXT]);
  p.Disc(cx, cy, 79, c[WIND_FACE]);
  if (d.heading_ok) {
    for (int a = 0; a < 360; a += 30) {
      const double rel = Signed180(a - Shown(d, hdg));
      p.Text(a == 0 ? std::string("N") : Three(a), Px(cx, rel, 89), Py(cy, rel, 89) + 5, a == 0 ? 17 : 14,
             c[WIND_FACE], Align::kCenter);
    }
    for (int a = 0; a < 360; a += 10) {
      const double rel = Signed180(a - Shown(d, hdg));
      p.Disc(Px(cx, rel, 119), Py(cy, rel, 119), a % 30 == 0 ? 2.2f : 1.8f, Mix(c[SECONDARY_TEXT], c[WIND_FACE], 0.35));
    }
  }
  for (int a = 0; a < 360; a += 30) {
    p.Line(Px(cx, a, 113), Py(cy, a, 113), Px(cx, a, 125), Py(cy, a, 125), c[WIND_TICKS], a % 90 == 0 ? 3.2f : 2.5f);
  }
  p.Line(cx, cy - 127, cx, cy - 112, c[WIND_TICKS], 5);

  const bool ok = d.true_ok && d.heading_ok;
  if (ok) {
    // The headings on each tack at a typical angle to the true wind: 45 degrees
    // upwind, 150 downwind. Green: starboard tack; red: port tack.
    const double target = std::fabs(d.twa) < 100 ? 45 : 150;
    const struct {
      double rel, half;
      Color fill, edge;
    } slices[2] = {{Signed180(d.twd - target - hdg), 8, Alpha(c[STARBOARD], 0.54), Alpha(c[STARBOARD], 0.85)},
                   {Signed180(d.twd + target - hdg), 10, Alpha(c[PORT], 0.53), Alpha(c[PORT], 0.9)}};
    for (const auto& s : slices) {
      std::vector<Pt> fan{{cx, cy}};
      for (int i = 0; i <= 8; ++i) {
        const double a = s.rel - s.half + 2 * s.half * i / 8;
        fan.push_back({Px(cx, a, 79), Py(cy, a, 79)});
      }
      p.Poly(fan.data(), static_cast<int>(fan.size()), s.fill);
      p.Line(cx, cy, Px(cx, s.rel + s.half, 79), Py(cy, s.rel + s.half, 79), s.edge, 4);
    }
  }
  Hull(p, pl, cx, 193.6f, 116);
  if (ok) {
    // The true wind arrow on the outer rose, pointing in.
    const double rel = Signed180(d.twd - hdg), ca = std::cos(rel * kDeg), sa = std::sin(rel * kDeg);
    const float bx = Px(cx, rel, 128), by = Py(cy, rel, 128);
    const float local[3][2] = {{-22 * k, -22 * k}, {22 * k, -22 * k}, {0, 36 * k}};
    Pt tri[3];
    for (int i = 0; i < 3; ++i) {
      tri[i] = {bx + static_cast<float>(local[i][0] * ca - local[i][1] * sa),
                by + static_cast<float>(local[i][0] * sa + local[i][1] * ca)};
    }
    p.Poly(tri, 3, c[ARROWS]);
    p.Text("T", bx, by + 7, 19, c[WIND_FACE], Align::kCenter);
  }
  const Pt mark1[3] = {{240.6f, 83.6f}, {257.6f, 83.6f}, {249.2f, 98.5f}};
  const Pt mark2[3] = {{249.2f, 123.3f}, {256.3f, 135.4f}, {242.1f, 135.4f}};
  p.Poly(mark1, 3, c[ARROW_SECONDARY]);
  p.Poly(mark2, 3, c[ARROW_SECONDARY]);
  p.Rect(226, 99, 47, 22, c[WIND_FACE], 6);
  p.Frame(226, 99, 47, 22, c[SECONDARY_TEXT], 1.4f, 6);
  p.Text(d.heading_ok ? Three(Shown(d, hdg)) : "---", 249.5f, 116, 16, c[PRIMARY_TEXT], Align::kCenter, false);

  p.Text(d.stw_ok ? Fmt("%.1f", d.stw) : "--.-", 29, 70, 47, c[PRIMARY_TEXT]);
  p.Text("BSPD", 29, 91, 17, c[SECONDARY_TEXT]);
  p.Text(d.true_ok ? Fmt("%.0f", std::fabs(d.twa)) : "--", 464, 70, 47, c[PRIMARY_TEXT], Align::kRight);
  p.Text("TWA", 464, 91, 17, c[SECONDARY_TEXT], Align::kRight);
  p.Text("TWD", 29, 303, 17, c[SECONDARY_TEXT]);
  p.Text(ok ? Three(Shown(d, d.twd)) : "---", 29, 348, 49, c[PRIMARY_TEXT]);
  p.Text("TWS", 464, 303, 17, c[SECONDARY_TEXT], Align::kRight);
  p.Text(d.true_ok ? Fmt("%.1f", d.tws) : "--.-", 464, 348, 47, c[PRIMARY_TEXT], Align::kRight);
  if (!d.heading_ok) Unavailable(p, pl, "No heading");
  else if (!d.true_ok) Unavailable(p, pl, "No wind data");
}

// ---------- Autopilot ----------

// The autopilot's semicircle: the heading box at the top, the scale and the
// deviation (starboard green, port red) with its marker.
void PilotArc(Painter& p, const Palette& pl, double current, double centre, double delta, bool marker) {
  const Color* c = pl.c;
  const float cx = 250, cy = 278;
  p.Band(cx, cy, 268.5f, 271.5f, -59.4f, 59.4f, c[STARBOARD]);
  p.Band(cx, cy, 246, 264, -59.4f, 59.4f, c[BEZEL]);
  const double signed_delta = std::fmax(-58.0, std::fmin(58.0, delta));
  if (marker && std::fabs(signed_delta) > 0.05) {
    p.Band(cx, cy, 246, 264, static_cast<float>(std::fmin(0.0, signed_delta)),
           static_cast<float>(std::fmax(0.0, signed_delta)), signed_delta > 0 ? c[STARBOARD] : c[PORT]);
  }
  for (int i = 0; i <= 6; ++i) {
    const double a = -55.8 + i * 18.6;
    p.Line(Px(cx, a, 246), Py(cy, a, 246), Px(cx, a, 265), Py(cy, a, 265), c[WIND_TICKS], 2);
  }
  p.Rect(217, 5, 67, 32, c[BEZEL]);
  p.Text(std::isfinite(current) ? Three(current) : "---", 250, 30, 23, c[WIND_TICKS], Align::kCenter);
  p.Text(Three(centre), 250, 82, 20, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(Three(centre - 30), 94, 139, 20, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(Three(centre + 30), 406, 139, 20, c[PRIMARY_TEXT], Align::kCenter);
  if (marker && std::fabs(signed_delta) > 0.05) {
    const double a = signed_delta;
    const float tx = static_cast<float>(std::cos(a * kDeg)), ty = static_cast<float>(std::sin(a * kDeg));
    const float bx = Px(cx, a, 215), by = Py(cy, a, 215);
    const Pt tri[3] = {{Px(cx, a, 243), Py(cy, a, 243)}, {bx + tx * 11, by + ty * 11}, {bx - tx * 11, by - ty * 11}};
    p.Poly(tri, 3, signed_delta > 0 ? c[STARBOARD] : c[PORT]);
  }
}

// The mode badge: the pilot's mode when known, AUTO when it steers a course, else STANDBY.
void PilotBadge(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  std::string label = d.pilot_mode, code = d.pilot_code;
  if (label.empty()) {
    label = d.pilot_ok ? "AUTO" : "STANDBY";
    code = d.pilot_ok ? "A" : "S";
  }
  const Color face = d.pilot_ok ? c[STARBOARD] : c[SECONDARY_TEXT];
  const float x = 7, y = 168, w = 142, centre = x + w / 2;
  p.Rect(x, y, w, 144, face);
  p.Rect(x + 6, y + 45, w - 12, 92, Dim(face, 0.75));
  std::string first = label, second;
  if (label == "NO DRIFT") {
    first = "NO";
    second = "DRIFT";
  } else if (label.size() > 9 && label.rfind(' ') != std::string::npos) {
    // "NON FOLLOW UP": before the last word and the last word.
    first = label.substr(0, label.rfind(' '));
    second = label.substr(label.rfind(' ') + 1);
  }
  if (second.empty()) {
    p.Text(first, centre, y + 31, 14, c[DISPLAY_BG], Align::kCenter);
  } else {
    p.Text(first, centre, y + 22, 14, c[DISPLAY_BG], Align::kCenter);
    p.Text(second, centre, y + 39, 14, c[DISPLAY_BG], Align::kCenter);
  }
  p.Text(code, centre, y + 122, code.size() > 2 ? 54.0f : 76.0f, c[DISPLAY_BG], Align::kCenter, false);
  if (!d.pilot_response.empty()) {
    p.Rect(5, 5, 67, 28, c[SECONDARY_TEXT]);
    p.Text(d.pilot_response, 39, 25, 15, c[DISPLAY_BG], Align::kCenter);
  }
}

// The rudder bar at the bottom: port red to the left, starboard green to the right.
void RudderBar(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Line(73, 349, 429, 349, Mix(c[GRID], c[SECONDARY_TEXT], 0.3), 3);
  const double angle = d.rudder_ok ? std::fmax(-45.0, std::fmin(45.0, d.rudder)) : 0;
  const float extent = static_cast<float>(std::fabs(angle) / 45 * 143);
  if (angle < 0) p.Rect(216 - extent, 345, extent, 8, c[PORT]);
  else if (angle > 0) p.Rect(284, 345, extent, 8, c[STARBOARD]);
  p.Rect(220, 333, 62, 31, c[BEZEL]);
  char text[8];
  std::snprintf(text, sizeof(text), "%02d", static_cast<int>(std::lround(std::fabs(angle))));
  p.Text(d.rudder_ok ? text : "--", 251, 357, 20, c[WIND_TICKS], Align::kCenter);
}

void Autopilot(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const double course = d.pilot_ok ? Shown(d, d.pilot_course) : NAN;
  const double ref = d.pilot_ok ? course : d.heading_ok ? Shown(d, d.hdg) : 0;
  const double delta = d.pilot_ok && d.heading_ok ? Signed180(d.pilot_course - d.hdg) : 0;
  PilotArc(p, pl, course, std::round(ref / 30) * 30, delta, d.pilot_ok && d.heading_ok);
  PilotBadge(p, pl, d);
  p.Text("Set Course", 266, 157, 23, c[SECONDARY_TEXT], Align::kCenter, false);
  p.Text(d.pilot_ok ? Three(course) : "---", 326, 278, 120, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(Ref(d), 477, 174, 22, c[SECONDARY_TEXT], Align::kRight, false);
  RudderBar(p, pl, d);
  if (!d.pilot_ok && !d.heading_ok) Unavailable(p, pl, "No autopilot data");
}

void Rudder(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const double hdg = d.heading_ok ? Shown(d, d.hdg) : NAN;
  PilotArc(p, pl, hdg, std::round((d.heading_ok ? hdg : 0) / 30) * 30, d.rudder_ok ? d.rudder : 0, d.rudder_ok);
  PilotBadge(p, pl, d);
  p.Text("Rudder Angle", 268, 138, 23, c[SECONDARY_TEXT], Align::kCenter, false);
  p.Text(d.rudder_ok ? Fmt("%.0f", std::fabs(d.rudder)) : "--", 328, 277, 110, c[PRIMARY_TEXT], Align::kCenter);
  p.Text("\xc2\xb0", 477, 173, 22, c[SECONDARY_TEXT], Align::kRight, false);
  RudderBar(p, pl, d);
  if (!d.rudder_ok) Unavailable(p, pl, "No rudder angle");
}

// ---------- Steering ----------

void Steering(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const float cx = 250, cy = 171, r = 247;
  const double hdg = d.heading_ok ? d.hdg : 0;
  p.Band(cx, cy, r - 1.5f, r + 1.5f, -70, 70, c[STARBOARD]);
  p.Band(cx, cy, r - 23, r - 3, -69, 69, c[BEZEL]);
  const bool w = d.waypoint_ok, h = d.heading_ok;
  if (w && h) {
    const double delta = std::fmax(-60.0, std::fmin(60.0, Signed180(d.btw - hdg)));
    if (std::fabs(delta) > 0.5) {
      p.Band(cx, cy, r - 23, r - 3, static_cast<float>(std::fmin(0.0, delta)), static_cast<float>(std::fmax(0.0, delta)),
             delta < 0 ? c[PORT] : c[STARBOARD]);
    }
  }
  // The arc runs above the screen, as in the studio; only the ticks on the screen are drawn.
  for (int a = -60; a <= 60; a += 10) {
    if (Py(cy, a, r - 5) < 0) continue;
    p.Line(Px(cx, a, r - 25), Py(cy, a, r - 25), Px(cx, a, r - 5), Py(cy, a, r - 5), c[WIND_TICKS], a % 30 == 0 ? 4.0f : 2.0f);
  }
  p.Rect(218, 1, 64, 30, c[BEZEL]);
  p.Text(h ? Three(Shown(d, hdg)) : "---", 250, 25, 23, c[WIND_TICKS], Align::kCenter);
  p.Text(d.position_ok ? Three(Shown(d, d.cog)) : "---", 250, 51, 22, c[PRIMARY_TEXT], Align::kCenter);
  const double centre = std::round(Shown(d, hdg) / 30) * 30;
  p.Text(Three(centre - 30), 72, 108, 21, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(Three(centre + 30), 426, 108, 21, c[PRIMARY_TEXT], Align::kCenter);
  if (w && h) {
    // The bearing to the waypoint on the scale.
    const double a = std::fmax(-62.0, std::fmin(62.0, Signed180(d.btw - hdg)));
    const Pt tri[3] = {{Px(cx, a, 172), Py(cy, a, 172)}, {Px(cx, a - 7, 132), Py(cy, a - 7, 132)},
                       {Px(cx, a + 7, 132), Py(cy, a + 7, 132)}};
    p.Poly(tri, 3, c[ARROWS]);
  }
  p.Line(0, 119, 500, 119, c[SEPARATOR], 1);
  p.Text("WPT BRG", 8, 150, 21, c[SECONDARY_TEXT]);
  p.Text(Ref(d), 157, 150, 18, c[SECONDARY_TEXT], Align::kRight, false);
  p.Text("XTE", 171, 150, 20, c[SECONDARY_TEXT]);
  p.Text("COG", 345, 150, 20, c[SECONDARY_TEXT]);
  p.Text(Ref(d), 491, 150, 18, c[SECONDARY_TEXT], Align::kRight, false);
  p.Line(164, 121, 164, 218, c[SEPARATOR], 1);
  p.Line(336, 121, 336, 218, c[SEPARATOR], 1);
  p.Text(w ? Three(Shown(d, d.btw)) : "---", 7, 213, 70, c[PRIMARY_TEXT]);
  p.Text(w ? Xte(d.xte, 2) : "-.--", 250, 205, 50, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(d.position_ok ? Three(Shown(d, d.cog)) : "---", 493, 213, 70, c[PRIMARY_TEXT], Align::kRight);
  p.Line(0, 223, 500, 223, c[SEPARATOR], 1);

  // Cross-track view: the track in red, the boat beside it by the XTE.
  p.Rect(109, 229, 282, 141, Mix(c[SECONDARY_TEXT], c[DISPLAY_BG], 0.3));
  const double scale = Scale(w ? d.xte : 0, {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5});
  const float bx = 250 + static_cast<float>(w ? std::fmax(-1.0, std::fmin(1.0, d.xte / scale)) * 128 : 0);
  p.Line(250, 229, 250, 370, c[PORT], 6);
  Dashed(p, bx, 229, 370, c[PRIMARY_TEXT], 2);
  p.Line(bx, 273, bx, 230, Mix(c[ARROWS], c[DISPLAY_BG], 0.35), 3);
  Hull(p, pl, bx, 309.7f, 71.4f);
  p.Text(Fmt(scale < 0.1 ? "%.2f" : "%.1f", scale), 24, 278, 24, c[PRIMARY_TEXT]);
  p.Text("NM", 25, 311, 24, c[PRIMARY_TEXT]);
  p.Text(Fmt(scale < 0.1 ? "%.2f" : "%.1f", scale), 475, 278, 24, c[PRIMARY_TEXT], Align::kRight);
  p.Text("NM", 475, 311, 24, c[PRIMARY_TEXT], Align::kRight);
  if (!h) Unavailable(p, pl, "No heading");
  else if (!w) Unavailable(p, pl, "No active waypoint");
}

// ---------- Tide ----------

void Tide(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  p.Line(168, 8, 168, 365, c[SEPARATOR], 1);
  p.Line(14, 128, 154, 128, c[SEPARATOR], 1);
  p.Line(14, 247, 154, 247, c[SEPARATOR], 1);
  p.Text("BOAT SPD", 11, 30, 21, c[SECONDARY_TEXT]);
  p.Text("kn", 158, 30, 20, c[SECONDARY_TEXT], Align::kRight);
  p.Text(d.stw_ok ? Fmt("%.1f", d.stw) : "--.-", 10, 113, 70, c[PRIMARY_TEXT]);
  p.Text("HDG", 11, 157, 21, c[SECONDARY_TEXT]);
  p.Text(Ref(d), 158, 157, 21, c[SECONDARY_TEXT], Align::kRight);
  p.Text(d.heading_ok ? Three(Shown(d, d.hdg)) : "---", 10, 236, 70, c[PRIMARY_TEXT]);
  p.Text("DEPTH", 11, 276, 21, c[SECONDARY_TEXT]);
  p.Text("m", 158, 276, 21, c[SECONDARY_TEXT], Align::kRight);
  p.Text(d.depth_ok ? Fmt("%.1f", d.depth) : "--.-", 10, 359, 70, c[PRIMARY_TEXT]);
  p.Text("TIDE", 176, 63, 50, c[SECONDARY_TEXT]);
  Hull(p, pl, 326, 170, 286);
  if (d.tide_ok) {
    // The current, where it flows to relative to the bow; the tail fades as in the studio.
    const double rel = Signed180(d.tide_set - d.hdg);
    const float fx = static_cast<float>(std::sin(rel * kDeg)), fy = static_cast<float>(-std::cos(rel * kDeg));
    const float sx = -fy, sy = fx, l = 245, wd = 112, ox = 328, oy = 185;
    const auto at = [&](float x, float y) { return Pt{ox + sx * x + fx * y, oy + sy * x + fy * y}; };
    const Pt tail[4] = {at(-wd * .22f, -l * .52f), at(wd * .22f, -l * .52f), at(wd * .22f, -l * .22f),
                        at(-wd * .22f, -l * .22f)};
    const Pt mid[4] = {at(-wd * .22f, -l * .22f), at(wd * .22f, -l * .22f), at(wd * .22f, l * .08f),
                       at(-wd * .22f, l * .08f)};
    const Pt head[3] = {at(wd * .52f, l * .08f), at(0, l * .54f), at(-wd * .52f, l * .08f)};
    p.Poly(tail, 4, Mix(c[ARROWS], c[DISPLAY_BG], 0.6));
    p.Poly(mid, 4, Mix(c[ARROWS], c[DISPLAY_BG], 0.25));
    p.Poly(head, 3, c[ARROWS]);
  }
  p.Text("kn", 177, 305, 23, c[PRIMARY_TEXT]);
  p.Text(d.tide_ok ? Fmt("%.1f", d.tide_drift) : "-.-", 177, 358, 48, c[PRIMARY_TEXT]);
  p.Text(Ref(d), 484, 305, 23, c[PRIMARY_TEXT], Align::kRight);
  p.Text(d.tide_ok ? Three(Shown(d, d.tide_set)) : "---", 484, 358, 48, c[PRIMARY_TEXT], Align::kRight);
  if (!d.tide_ok) Unavailable(p, pl, "No tide data");
}

// ---------- Barometer and Weather ----------

void Barometer(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const float x = 58, y = 57, w = 424, h = 180;
  double min, max;
  PressureRange(d, &min, &max);
  p.Text("BAROMETER", 250, 42, 26, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(d.pressure_ok ? Fmt("%.1f mb", d.pressure) : "----.- mb", 480, 42, 22, c[PRIMARY_TEXT], Align::kRight);
  const Color grid = Mix(c[GRID], c[SECONDARY_TEXT], 0.4);
  for (int i = 0; i <= 4; ++i) {
    const float gy = y + i * h / 4;
    p.Line(x, gy, x + w, gy, grid, 1);
    p.Text(Fmt("%.0f", max - (max - min) * i / 4), x - 7, gy + 5, 13, c[SECONDARY_TEXT], Align::kRight);
  }
  for (int i = 0; i <= 5; ++i) {
    const float gx = x + i * w / 5;
    p.Line(gx, y, gx, y + h, grid, 1);
    p.Text(std::to_string(5 - i), gx, y + h + 19, 13, c[SECONDARY_TEXT], Align::kCenter);
  }
  AreaGraph(p, pl, d.pressure_history, 300, x, y, w, h, min, max, 3);
  p.Line(58, 286, 482, 286, c[SEPARATOR], 1);
  p.Line(200, 288, 200, 365, c[SEPARATOR], 1);
  p.Line(348, 288, 348, 365, c[SEPARATOR], 1);
  p.Text("AIR TEMP \xc2\xb0" "C", 65, 310, 17, c[PRIMARY_TEXT]);
  p.Text(d.air_temp_ok ? Fmt("%.1f", d.air_temp) : "--.-", 65, 361, 44, c[PRIMARY_TEXT]);
  p.Text(std::string("TWD ") + Ref(d), 209, 310, 17, c[PRIMARY_TEXT]);
  p.Text(d.true_ok && d.heading_ok ? Three(Shown(d, d.twd)) : "---", 209, 361, 44, c[PRIMARY_TEXT]);
  p.Text("TWS kn", 355, 310, 17, c[PRIMARY_TEXT]);
  p.Text(d.true_ok ? Fmt("%.1f", d.tws) : "--.-", 355, 361, 44, c[PRIMARY_TEXT]);
  if (!d.pressure_ok && d.pressure_history.empty()) Unavailable(p, pl, "No pressure data");
}

void Weather(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const float x = 53, y = 39, w = 432, h = 184;
  double min, max;
  PressureRange(d, &min, &max);
  p.Text("BAROMETER", 250, 31, 25, c[SECONDARY_TEXT], Align::kCenter);
  p.Text(d.pressure_ok ? Fmt("%.1f mb", d.pressure) : "----.- mb", 486, 31, 23, c[PRIMARY_TEXT], Align::kRight);
  const Color grid = Mix(c[SECONDARY_TEXT], c[DISPLAY_BG], 0.15);
  for (int i = 0; i < 4; ++i) {
    const float gy = y + i * h / 3;
    p.Line(x, gy, x + w, gy, grid, 1.4f);
    p.Text(Fmt("%.0f", max - (max - min) * i / 3), x - 8, gy + 7, 18, c[PRIMARY_TEXT], Align::kRight, false);
  }
  for (int i = 0; i <= 3; ++i) {
    const float gx = x + i * w / 3;
    p.Line(gx, y, gx, y + h, grid, 1.4f);
    p.Text(std::to_string(3 - i), gx, y + h + 23, 17, c[PRIMARY_TEXT], Align::kCenter, false);
  }
  AreaGraph(p, pl, d.pressure_history, 180, x, y, w, h, min, max, 4);
  p.Line(0, 249, 500, 249, c[SEPARATOR], 1);
  p.Line(168, 249, 168, 370, c[SEPARATOR], 1);
  p.Line(336, 249, 336, 370, c[SEPARATOR], 1);
  p.Text("AIR TEMP", 8, 280, 19, c[SECONDARY_TEXT]);
  p.Text("\xc2\xb0" "C", 159, 280, 18, c[SECONDARY_TEXT], Align::kRight);
  p.Text("TWD", 177, 280, 19, c[SECONDARY_TEXT]);
  p.Text(Ref(d), 328, 280, 18, c[SECONDARY_TEXT], Align::kRight);
  p.Text("TWS", 345, 280, 19, c[SECONDARY_TEXT]);
  p.Text("kn", 493, 280, 18, c[SECONDARY_TEXT], Align::kRight);
  p.Text(d.air_temp_ok ? Fmt("%.1f", d.air_temp) : "--.-", 12, 358, 62, c[PRIMARY_TEXT]);
  p.Text(d.true_ok && d.heading_ok ? Three(Shown(d, d.twd)) : "---", 177, 358, 62, c[PRIMARY_TEXT]);
  p.Text(d.true_ok ? Fmt("%.1f", d.tws) : "--.-", 345, 358, 62, c[PRIMARY_TEXT]);
  if (!d.pressure_ok && d.pressure_history.empty()) Unavailable(p, pl, "No pressure data");
}

// ---------- SOG / COG history ----------

void SogCog(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const float x = 48, w = 438, top1 = 31, h = 112, top2 = 202, bottom2 = 348;
  const Color grid = Mix(c[SECONDARY_TEXT], c[DISPLAY_BG], 0.1);
  p.Text("SOG", 250, 24, 25, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(d.position_ok ? Fmt("%.1f kn", d.sog) : "--.- kn", 488, 24, 25, c[PRIMARY_TEXT], Align::kRight);
  p.Text("COG", 250, 191, 25, c[PRIMARY_TEXT], Align::kCenter);
  p.Text((d.position_ok ? Three(Shown(d, d.cog)) : std::string("---")) + " " + Ref(d), 488, 191, 25, c[PRIMARY_TEXT],
         Align::kRight);
  double peak = d.position_ok ? d.sog : 0;
  for (float v : d.sog_history) peak = std::fmax(peak, v);
  const double sog_max = std::fmax(10.0, std::ceil(peak / 5) * 5);
  for (int i = 0; i <= 2; ++i) {
    const double v = sog_max * i / 2;
    const float y = top1 + h - static_cast<float>(v / sog_max) * h;
    p.Line(x, y, x + w, y, grid, 1);
    p.Text(Fmt("%.0f", v), 42, y + 6, 20, c[PRIMARY_TEXT], Align::kRight);
  }
  for (int m = 0; m <= 5; ++m) {
    const float xx = x + w * (1 - m / 5.0f);
    p.Line(xx, top1, xx, top1 + h, grid, 1);
    p.Text(std::to_string(m), xx, 163, 20, c[PRIMARY_TEXT], Align::kCenter);
  }
  AreaGraph(p, pl, d.sog_history, 300, x, top1, w, h, 0, sog_max, 3);
  for (int v = 0; v <= 360; v += 90) {
    const float y = bottom2 - (v / 360.0f) * (bottom2 - top2);
    p.Line(x, y, x + w, y, grid, 1);
    p.Text(std::to_string(v), 42, y + 6, 20, c[PRIMARY_TEXT], Align::kRight);
  }
  for (int m = 0; m <= 5; ++m) {
    const float xx = x + w * (1 - m / 5.0f);
    p.Line(xx, top2, xx, bottom2, grid, 1);
    p.Text(std::to_string(m), xx, 368, 20, c[PRIMARY_TEXT], Align::kCenter);
  }
  const std::vector<float>& cog = d.cog_history;
  for (size_t i = 1; i < cog.size(); ++i) {
    const double a = std::fmod(Shown(d, cog[i - 1]) + 360, 360), b = std::fmod(Shown(d, cog[i]) + 360, 360);
    if (std::fabs(a - b) > 180) continue;  // Across north: no line from 359 to 0.
    p.Line(AgeX(x, w, i - 1, cog.size(), 300), bottom2 - static_cast<float>(a / 360) * (bottom2 - top2),
           AgeX(x, w, i, cog.size(), 300), bottom2 - static_cast<float>(b / 360) * (bottom2 - top2), c[ARROWS], 3);
  }
  if (!d.position_ok && d.sog_history.empty()) Unavailable(p, pl, "No position");
}

// ---------- Wind History and Wind plot ----------

void WindHistory(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const bool ok = d.true_ok && d.heading_ok;
  const double twd = ok ? Shown(d, d.twd) : 0, tws = d.true_ok ? d.tws : 0;
  const float top = 78, h = 275;
  p.Text("TWD", 8, 31, 26, c[PRIMARY_TEXT]);
  p.Text(ok ? Three(twd) : "---", 151, 53, 50, c[PRIMARY_TEXT], Align::kCenter);
  p.Text(Ref(d), 245, 30, 24, c[PRIMARY_TEXT], Align::kRight);
  p.Text("TWS", 278, 31, 26, c[PRIMARY_TEXT]);
  p.Text(d.true_ok ? Fmt("%.1f", tws) : "--.-", 402, 53, 50, c[PRIMARY_TEXT], Align::kCenter);
  p.Text("kn", 492, 30, 24, c[PRIMARY_TEXT], Align::kRight);

  // Direction: 40 degrees around the present one; speed: the range of the last ten minutes.
  const double dir_min = std::floor((twd - 20) / 10) * 10, dir_max = dir_min + 40;
  double lo = tws, hi = tws;
  for (float v : d.tws_history) {
    lo = std::fmin(lo, v);
    hi = std::fmax(hi, v);
  }
  double speed_min = std::fmax(0.0, std::floor(lo - 1)), speed_max = std::ceil(hi + 1);
  if (speed_max - speed_min < 3) speed_max = speed_min + 3;
  const auto panel = [&](float bx, float bw, double min, double max, const std::vector<float>& values, double now,
                         bool direction, int decimals) {
    p.Frame(bx, top, bw, h, c[GRID], 1.4f);
    for (int m = 0; m <= 10; m += 2) p.Line(bx, top + h * m / 10, bx + bw, top + h * m / 10, Mix(c[GRID], c[SECONDARY_TEXT], 0.3), 1);
    const auto map = [&](double v) { return bx + static_cast<float>((std::fmax(min, std::fmin(max, v)) - min) / (max - min)) * bw; };
    const float nx = map(now);
    p.Line(nx, top, nx, top + h, c[PORT], 1.2f);
    std::vector<Pt> pts;
    for (size_t i = 0; i < values.size(); ++i) {
      double v = direction ? Shown(d, values[i]) : values[i];
      if (direction) v = now + Signed180(v - now);
      const float age = static_cast<float>(values.size() - 1 - i) / 299.0f;  // 0 now, 1 ten minutes ago.
      pts.push_back({map(v), top + h * age});
    }
    Polyline(p, pts, c[ARROWS], 3);
    const char* f = decimals ? "%.1f" : "%.0f";
    p.Text(Fmt(f, direction ? std::fmod(min + 360, 360) : min), bx + 3, 70, 20, c[PRIMARY_TEXT]);
    p.Text(Fmt(f, direction ? std::fmod(now + 360, 360) : now), nx, 70, 20, c[PORT], Align::kCenter);
    p.Text(Fmt(f, direction ? std::fmod(max + 360, 360) : max), bx + bw - 3, 70, 20, c[PRIMARY_TEXT], Align::kRight);
  };
  panel(8, 205, dir_min, dir_max, d.twd_history, twd, true, 0);
  panel(276, 216, speed_min, speed_max, d.tws_history, tws, false, 1);
  for (int m = 0; m <= 10; m += 2) p.Text(std::to_string(m), 250, top + h * m / 10 + 7, 21, c[PRIMARY_TEXT], Align::kCenter);
  if (!d.true_ok) Unavailable(p, pl, "No wind data");
}

void WindPlot(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const bool ok = d.true_ok && d.heading_ok;
  p.Text("TWD", 15, 32, 22, c[SECONDARY_TEXT]);
  p.Text((ok ? Three(Shown(d, d.twd)) : std::string("---")) + " " + Ref(d), 164, 32, 24, c[PRIMARY_TEXT], Align::kCenter);
  p.Text("TWS", 269, 32, 22, c[SECONDARY_TEXT]);
  p.Text(d.true_ok ? Fmt("%.1f kn", d.tws) : "--.- kn", 478, 32, 23, c[PRIMARY_TEXT], Align::kRight);
  p.Line(251, 41, 251, 355, c[SEPARATOR], 1);
  double tws_max = 30;
  for (float v : d.tws_history) while (v > tws_max) tws_max += 10;
  const struct {
    float x, w;
    double min, max;
    const std::vector<float>* values;
    bool direction;
  } sets[2] = {{39, 192, 0, 360, &d.twd_history, true}, {273, 192, 0, tws_max, &d.tws_history, false}};
  for (const auto& s : sets) {
    for (int i = 0; i <= 5; ++i) {
      const float y = 49 + i * 60.0f;
      p.Line(s.x, y, s.x + s.w, y, c[GRID], 1);
      p.Text(std::to_string(i * 2), s.x - 8, y + 5, 13, c[SECONDARY_TEXT], Align::kRight);
    }
    const std::vector<float>& v = *s.values;
    for (size_t i = 1; i < v.size(); ++i) {
      const double a = s.direction ? std::fmod(Shown(d, v[i - 1]) + 360, 360) : v[i - 1];
      const double b = s.direction ? std::fmod(Shown(d, v[i]) + 360, 360) : v[i];
      if (s.direction && std::fabs(a - b) > 180) continue;
      const float y0 = 49 + 300.0f * static_cast<float>(v.size() - i) / 299.0f;
      const float y1 = 49 + 300.0f * static_cast<float>(v.size() - 1 - i) / 299.0f;
      p.Line(s.x + static_cast<float>((a - s.min) / (s.max - s.min)) * s.w, y0,
             s.x + static_cast<float>((b - s.min) / (s.max - s.min)) * s.w, y1, c[ARROWS], 2.5f);
    }
  }
  if (!d.true_ok) Unavailable(p, pl, "No wind data");
}
