#include "md_pages.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace md {

namespace {

struct PageInfo {
  Page page;
  const char* key;
  const char* name;
  const char* full;
};
// Indexed by Page.
const PageInfo kPages[kPageCount] = {
    {Page::kWindQuad, "windquad", "Wind Quad", "Wind Quad \xe2\x80\x94 apparent and true wind"},
    {Page::kSteer, "steer", "Sail Steer", "Sail Steer \xe2\x80\x94 heading, wind and waypoint"},
    {Page::kLaylines, "laylines", "Laylines", "Laylines \xe2\x80\x94 tacks on the true wind"},
    {Page::kWindHistory, "windhistory", "Wind History", "Wind History \xe2\x80\x94 TWD and TWS, 10 minutes"},
    {Page::kWindPlot, "windplot", "Wind plot", "Wind plot \xe2\x80\x94 TWD and TWS, 10 minutes"},
    {Page::kHighway, "highway", "Highway", "Highway \xe2\x80\x94 course to the waypoint"},
    {Page::kSteering, "steering", "Steering", "Steering \xe2\x80\x94 waypoint, XTE and COG"},
    {Page::kAutopilot, "autopilot", "Autopilot", "Autopilot \xe2\x80\x94 set course"},
    {Page::kRudder, "rudder", "Rudder", "Autopilot \xe2\x80\x94 rudder angle"},
    {Page::kTide, "tide", "Tide", "Tide \xe2\x80\x94 current set and drift"},
    {Page::kAis, "ais", "AIS", "AIS \xe2\x80\x94 targets around the boat"},
    {Page::kGps, "gps", "GPS", "GPS \xe2\x80\x94 position, COG and SOG"},
    {Page::kSogCog, "sogcog", "SOG / COG", "SOG / COG \xe2\x80\x94 last 5 minutes"},
    {Page::kDepth, "depth", "Depth", "Depth \xe2\x80\x94 now and last 5 minutes"},
    {Page::kBarometer, "barometer", "Barometer", "Barometer \xe2\x80\x94 last 5 hours"},
    {Page::kWeather, "weather", "Weather", "Weather \xe2\x80\x94 pressure, air and wind, 3 hours"}};
const char* const kDegree = "\xc2\xb0";

std::string Fmt(const char* format, double v) {
  char text[48];
  std::snprintf(text, sizeof(text), format, v);
  // printf follows the C locale, which OpenCPN may set to a comma decimal.
  for (char* c = text; *c; ++c) {
    if (*c == ',') *c = '.';
  }
  return text;
}

std::string Bearing(double deg) {
  char text[16];
  std::snprintf(text, sizeof(text), "%03d", static_cast<int>(std::lround(std::fmod(deg + 360, 360))) % 360);
  return std::string(text) + kDegree;
}

double Signed180(double a) {
  a = std::fmod(a + 540.0, 360.0);
  return a - 180.0;
}

const Color kWhite = Rgb(0xffffff);
const Color kBlack = Rgb(0x000000);

void Rose(Painter& p, const Palette& pl, float cx, float cy, float R, bool labels) {
  const Color* c = pl.c;
  const float rBand = R * 0.94f, rIn = R * 0.79f;
  p.Disc(cx, cy, R, c[BEZEL]);
  p.Band(cx, cy, rIn, rBand, 0, 359.99f, c[WIND_NEUTRAL]);
  // Pure red and green up to 42 degrees, fading to grey by 74 (the studio's defaults).
  for (int a = 0; a < 74; a += 2) {
    const double f = a < 42 ? 0 : (a - 42) / 32.0;
    p.Band(cx, cy, rIn, rBand, -a - 2.2f, static_cast<float>(-a), Mix(c[WIND_PORT], c[WIND_NEUTRAL], f));
    p.Band(cx, cy, rIn, rBand, static_cast<float>(a), a + 2.2f, Mix(c[WIND_STARBOARD], c[WIND_NEUTRAL], f));
  }
  p.Disc(cx, cy, rIn, c[WIND_FACE]);
  for (int a = 0; a < 360; a += 10) {
    const bool major = a % 30 == 0;
    const float r0 = major ? rIn + 2 : rIn + (rBand - rIn) * 0.45f;
    p.Line(Px(cx, a, r0), Py(cy, a, r0), Px(cx, a, rBand - 2), Py(cy, a, rBand - 2), c[WIND_TICKS], major ? 3.0f : 1.4f);
  }
  if (labels) {
    for (int a = 30; a <= 150; a += 30) {
      const std::string s = std::to_string(a);
      p.Text(s, Px(cx, a, rIn - 16), Py(cy, a, rIn - 16) + 5, 13, c[SECONDARY_TEXT], Align::kCenter, false);
      p.Text(s, Px(cx, -a, rIn - 16), Py(cy, -a, rIn - 16) + 5, 13, c[SECONDARY_TEXT], Align::kCenter, false);
    }
  }
}

void Hull(Painter& p, const Palette& pl, float cx, float cy, float h) {
  const float w = h * 0.36f;
  std::vector<Pt> pts;
  const auto half = [&](double t) {
    return w / 2 * std::pow(std::sin(std::fmin(1.0, t * 1.25) * 3.14159265 / 2), 0.9) * (t > 0.92 ? 0.94 : 1.0);
  };
  for (int i = 0; i <= 20; ++i) {
    const double t = i / 20.0;
    pts.push_back({static_cast<float>(cx + half(t)), static_cast<float>(cy - h / 2 + t * h)});
  }
  for (int i = 20; i >= 0; --i) {
    const double t = i / 20.0;
    pts.push_back({static_cast<float>(cx - half(t)), static_cast<float>(cy - h / 2 + t * h)});
  }
  p.Poly(pts.data(), static_cast<int>(pts.size()), pl.c[HULL], 3.2f);
}

void Wedge(Painter& p, float cx, float cy, double angle, float len, double half, Color color, const char* label) {
  const Pt pts[3] = {{cx, cy}, {Px(cx, angle - half, len), Py(cy, angle - half, len)},
                     {Px(cx, angle + half, len), Py(cy, angle + half, len)}};
  p.Poly(pts, 3, color);
  p.Text(label, Px(cx, angle, len * 0.8f), Py(cy, angle, len * 0.8f) + 8, 22, kWhite, Align::kCenter);
}

void Corner(Painter& p, const Palette& pl, float x, float y, float w, const std::string& value,
            const char* unit, bool top) {
  const Color* c = pl.c;
  p.Rect(x, y, w, 160, c[DISPLAY_BG], 8);
  if (top) {
    p.Text(value, x + w / 2, y + 62, 58, c[PRIMARY_TEXT], Align::kCenter);
    p.Text(unit, x + w / 2, y + 92, 22, c[PRIMARY_TEXT], Align::kCenter);
  } else {
    p.Text(unit, x + w / 2, y + 70, 22, c[PRIMARY_TEXT], Align::kCenter);
    p.Text(value, x + w / 2, y + 140, 58, c[PRIMARY_TEXT], Align::kCenter);
  }
}

void Unavailable(Painter& p, const Palette& pl, const char* message) {
  p.Rect(120, 150, 260, 70, pl.c[MENU], 8);
  p.Text(message, 250, 193, 22, pl.c[MENU_TEXT], Align::kCenter);
}

void WindQuad(Painter& p, const Palette& pl, const Data& d) {
  p.Rect(0, 0, kPageWidth, kPageHeight, kBlack);
  const bool a = d.apparent_ok, t = d.true_ok;
  Corner(p, pl, 6, 6, 112, a ? Fmt("%.1f", d.aws) : "--.-", "AWS", true);
  Corner(p, pl, 382, 6, 112, a ? Fmt("%.0f", std::fabs(d.awa)) : "---", "AWA", true);
  Corner(p, pl, 6, 204, 112, t ? Fmt("%.1f", d.tws) : "--.-", "TWS", false);
  Corner(p, pl, 382, 204, 112, t ? Fmt("%.0f", std::fabs(d.twa)) : "---", "TWA", false);
  Rose(p, pl, 250, 185, 166, false);
  Hull(p, pl, 250, 188, 170);
  if (t) Wedge(p, 250, 185, d.twa, 128, 7, pl.c[ARROW_SECONDARY], "T");
  if (a) Wedge(p, 250, 185, d.awa, 128, 9, pl.c[ARROWS], "A");
  if (!a && !t) Unavailable(p, pl, "No wind data");
}

void Steer(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const float cx = 250, cy = 196, R = 158;
  Rose(p, pl, cx, cy, R, true);
  Hull(p, pl, cx, cy + 4, 150);
  if (d.true_ok) Wedge(p, cx, cy, d.twa, 120, 7, c[ARROW_SECONDARY], "T");
  if (d.heading_ok && d.waypoint_ok) {
    const double rel = Signed180(d.btw - d.hdg);
    const float r0 = R * 0.79f - 2, r1 = R * 0.62f;
    const Pt pts[3] = {{Px(cx, rel, r0), Py(cy, rel, r0)}, {Px(cx, rel - 5, r1), Py(cy, rel - 5, r1)},
                       {Px(cx, rel + 5, r1), Py(cy, rel + 5, r1)}};
    p.Poly(pts, 3, c[STARBOARD]);
  }
  p.Text("HDG", 14, 30, 16, c[SECONDARY_TEXT]);
  p.Text(d.heading_ok ? Bearing(d.hdg) : std::string("---") + kDegree, 14, 66, 38, c[PRIMARY_TEXT]);
  p.Text("BTW", 486, 30, 16, c[SECONDARY_TEXT], Align::kRight);
  p.Text(d.waypoint_ok ? Bearing(d.btw) : std::string("---") + kDegree, 486, 66, 38, c[PRIMARY_TEXT], Align::kRight);
  p.Text("DTW", 14, 322, 16, c[SECONDARY_TEXT]);
  p.Text(d.waypoint_ok ? Fmt("%.2f NM", d.dtw) : "-.-- NM", 14, 356, 30, c[PRIMARY_TEXT]);
  p.Text("XTE", 486, 322, 16, c[SECONDARY_TEXT], Align::kRight);
  p.Text(d.waypoint_ok ? Fmt("%.2f NM", std::fabs(d.xte)) + (d.xte < 0 ? " L" : d.xte > 0 ? " R" : "") : "-.--",
         486, 356, 30, c[PRIMARY_TEXT], Align::kRight);
  if (!d.heading_ok) Unavailable(p, pl, "No heading");
}

void Ais(Painter& p, const Palette& pl, const Data& d, PageState& s) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const float cx = 190, cy = 190, R = 165;
  // The smallest range that holds the nearest target, as the firmware's Auto range.
  static const double kSteps[] = {1.5, 3, 6, 12};
  double nearest = 1e9;
  for (const AisTarget& t : d.targets) nearest = std::fmin(nearest, t.range);
  double range = 12;
  for (double step : kSteps) {
    if (nearest <= step * 0.9) {
      range = step;
      break;
    }
  }
  for (int i = 1; i <= 3; ++i) p.Ring(cx, cy, R * i / 3, c[AIS_RINGS], 1.2f);
  p.Line(cx, cy, cx, cy - R, c[AIS_RINGS], 1);
  p.Text(Fmt(range < 3 ? "%.1f NM" : "%.0f NM", range) + (d.heading_ok ? " \xc2\xb7 head up" : " \xc2\xb7 course up"),
         12, 22, 15, c[SECONDARY_TEXT]);
  for (int i = 1; i <= 3; ++i) {
    p.Text(Fmt(range < 3 ? "%.1f" : "%.0f", range * i / 3), cx + 4, cy - R * i / 3 + 14, 12, c[SECONDARY_TEXT],
           Align::kLeft, false);
  }
  const Pt own[4] = {{cx, cy - 12}, {cx - 8, cy + 9}, {cx, cy + 4}, {cx + 8, cy + 9}};
  p.Poly(own, 4, c[AIS_OWN]);
  if (!d.ais_ok || !d.position_ok) {
    Unavailable(p, pl, !d.position_ok ? "No position" : "No AIS data");
    return;
  }
  const double up = d.heading_ok ? d.hdg : d.cog;
  std::vector<const AisTarget*> shown;
  for (const AisTarget& t : d.targets) {
    if (t.range <= range * 1.02) shown.push_back(&t);
  }
  std::sort(shown.begin(), shown.end(), [](const AisTarget* a, const AisTarget* b) { return a->range < b->range; });
  if (s.ais_selected >= static_cast<int>(shown.size()) || s.ais_selected < 0) s.ais_selected = 0;
  for (size_t i = 0; i < shown.size(); ++i) {
    const AisTarget& t = *shown[i];
    const double brg = t.brg - up, dist = t.range / range * R;
    const float tx = Px(cx, brg, static_cast<float>(dist)), ty = Py(cy, brg, static_cast<float>(dist));
    const double crs = t.cog - up;
    const Color col = t.danger ? c[AIS_DANGER] : c[AIS_TARGET];
    if (t.sog < 0.5) {
      const Pt diamond[4] = {{tx, ty - 7}, {tx + 7, ty}, {tx, ty + 7}, {tx - 7, ty}};
      p.Poly(diamond, 4, col, 2);
    } else {
      const Pt tri[3] = {{Px(tx, crs, 12), Py(ty, crs, 12)}, {Px(tx, crs + 145, 9), Py(ty, crs + 145, 9)},
                         {Px(tx, crs - 145, 9), Py(ty, crs - 145, 9)}};
      p.Poly(tri, 3, col, t.danger ? 0 : 2);
      const float len = 12 + static_cast<float>(t.sog) * 3;
      p.Line(Px(tx, crs, 12), Py(ty, crs, 12), Px(tx, crs, len), Py(ty, crs, len), col, 1.4f);
    }
    if (static_cast<int>(i) == s.ais_selected) p.Ring(tx, ty, 15, c[ARROWS], 2);
  }
  p.Rect(372, 10, 120, 350, c[MENU], 6);
  if (shown.empty()) {
    p.Text("No targets", 432, 40, 15, c[MENU_TEXT], Align::kCenter);
    return;
  }
  const AisTarget& t = *shown[static_cast<size_t>(s.ais_selected)];
  std::string name = t.name.empty() ? std::to_string(t.mmsi) : t.name;
  if (name.size() > 14) name.resize(14);
  const Color alarm = t.danger ? c[AIS_DANGER] : c[MENU_TEXT];
  p.Text(name, 380, 34, 15, c[MENU_TEXT]);
  p.Text(std::to_string(t.mmsi), 380, 60, 13, c[MENU_TEXT], Align::kLeft, false);
  p.Text("BRG " + Bearing(t.brg), 380, 86, 14, c[MENU_TEXT], Align::kLeft, false);
  p.Text(Fmt("RNG %.2f NM", t.range), 380, 112, 14, c[MENU_TEXT], Align::kLeft, false);
  p.Text("COG " + Bearing(t.cog), 380, 138, 14, c[MENU_TEXT], Align::kLeft, false);
  p.Text(Fmt("SOG %.1f kn", t.sog), 380, 164, 14, c[MENU_TEXT], Align::kLeft, false);
  p.Text(t.cpa_valid ? Fmt("CPA %.2f NM", t.cpa) : "CPA --", 380, 190, 14, alarm, Align::kLeft, false);
  p.Text(t.cpa_valid ? Fmt("TCPA %.0f min", t.tcpa) : "TCPA --", 380, 216, 14, alarm, Align::kLeft, false);
  p.Text(std::to_string(s.ais_selected + 1) + " / " + std::to_string(shown.size()), 432, 320, 13, c[MENU_TEXT],
         Align::kCenter, false);
  p.Text("ENTER: next", 432, 346, 12, c[MENU_TEXT], Align::kCenter, false);
}

std::string DegMin(double v, bool lat) {
  const char hemi = lat ? (v >= 0 ? 'N' : 'S') : (v >= 0 ? 'E' : 'W');
  v = std::fabs(v);
  const int deg = static_cast<int>(v);
  const double min = (v - deg) * 60;
  char text[40];
  std::snprintf(text, sizeof(text), lat ? "%02d" : "%03d", deg);
  return std::string(text) + kDegree + " " + Fmt("%06.3f", min) + "\xe2\x80\xb2 " + hemi;
}

void Gps(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  const bool ok = d.position_ok;
  p.Text("LATITUDE", 24, 46, 17, c[SECONDARY_TEXT]);
  p.Text(ok ? DegMin(d.lat, true) : std::string("--") + kDegree + " --.---", 24, 100, 54, c[PRIMARY_TEXT]);
  p.Line(20, 124, 480, 124, c[SEPARATOR], 1.2f);
  p.Text("LONGITUDE", 24, 150, 17, c[SECONDARY_TEXT]);
  p.Text(ok ? DegMin(d.lon, false) : std::string("---") + kDegree + " --.---", 24, 204, 54, c[PRIMARY_TEXT]);
  p.Line(20, 228, 480, 228, c[SEPARATOR], 1.2f);
  p.Text("COG", 24, 262, 17, c[SECONDARY_TEXT]);
  p.Text(ok ? Bearing(d.cog) : std::string("---") + kDegree, 24, 334, 64, c[PRIMARY_TEXT]);
  p.Text("SOG", 270, 262, 17, c[SECONDARY_TEXT]);
  p.Text(ok ? Fmt("%.1f", d.sog) : "--.-", 270, 334, 64, c[PRIMARY_TEXT]);
  p.Text("kn", 476, 334, 20, c[SECONDARY_TEXT], Align::kRight);
}

void Depth(Painter& p, const Palette& pl, const Data& d) {
  const Color* c = pl.c;
  p.Rect(0, 0, kPageWidth, kPageHeight, c[DISPLAY_BG]);
  p.Text("DEPTH", 24, 44, 17, c[SECONDARY_TEXT]);
  p.Text(d.depth_ok ? Fmt("%.1f", d.depth) : "--.-", 24, 160, 120,
         d.depth_ok && d.depth < 3 ? c[WARNING] : c[PRIMARY_TEXT]);
  p.Text("m", 260, 160, 30, c[SECONDARY_TEXT]);
  const float x0 = 24, x1 = 476, y0 = 200, y1 = 350;
  p.Frame(x0, y0, x1 - x0, y1 - y0, c[GRID], 1.2f, 4);
  const std::vector<float>& h = d.depth_history;
  if (h.size() > 1) {
    float max = 15;
    for (float v : h) {
      if (v > max) max = v;
    }
    max *= 1.1f;
    std::vector<Pt> pts;
    pts.push_back({x1 - (x1 - x0) * static_cast<float>(h.size() - 1) / 299.0f, y0});
    for (size_t i = 0; i < h.size(); ++i) {
      const float age = static_cast<float>(h.size() - 1 - i);
      pts.push_back({x1 - (x1 - x0) * age / 299.0f, y0 + (y1 - y0) * h[i] / max});
    }
    pts.push_back({pts.back().x, y0});
    p.Poly(pts.data(), static_cast<int>(pts.size()), Dim(c[ARROWS], 0.9));
    p.Text(Fmt("%.0f m", max), x1 - 6, y1 - 8, 13, c[SECONDARY_TEXT], Align::kRight, false);
  }
  p.Text("5 min", x0 + 6, y1 - 8, 13, c[SECONDARY_TEXT], Align::kLeft, false);
}

#include "md_pages_more.inc"

}  // namespace

const char* PageName(Page page) { return kPages[static_cast<int>(page)].full; }
const char* PageShort(Page page) { return kPages[static_cast<int>(page)].name; }
const char* PageKey(Page page) { return kPages[static_cast<int>(page)].key; }

bool PageFromKey(const char* key, Page* page) {
  for (const PageInfo& info : kPages) {
    if (std::strcmp(key, info.key) == 0) {
      *page = info.page;
      return true;
    }
  }
  return false;
}

void DrawPage(Painter& p, const Palette& palette, const Data& data, PageState& state) {
  switch (state.page) {
    case Page::kWindQuad: WindQuad(p, palette, data); break;
    case Page::kSteer: Steer(p, palette, data); break;
    case Page::kAis: Ais(p, palette, data, state); break;
    case Page::kGps: Gps(p, palette, data); break;
    case Page::kDepth: Depth(p, palette, data); break;
    case Page::kLaylines: Laylines(p, palette, data); break;
    case Page::kWindHistory: WindHistory(p, palette, data); break;
    case Page::kWindPlot: WindPlot(p, palette, data); break;
    case Page::kHighway: Highway(p, palette, data); break;
    case Page::kSteering: Steering(p, palette, data); break;
    case Page::kAutopilot: Autopilot(p, palette, data); break;
    case Page::kRudder: Rudder(p, palette, data); break;
    case Page::kTide: Tide(p, palette, data); break;
    case Page::kSogCog: SogCog(p, palette, data); break;
    case Page::kBarometer: Barometer(p, palette, data); break;
    case Page::kWeather: Weather(p, palette, data); break;
  }
}

}  // namespace md
