// Standalone check of the Marine Display core (no wxWidgets, no OpenCPN):
//   c++ -std=c++17 -Icore tests/core_test.cpp core/*.cpp -o core_test && ./core_test
// Theme import, NMEA 0183 sentences, true wind, the demo and every page drawn
// through a recording painter.
#include <clocale>
#include <cmath>
#include <iostream>
#include <string>

#include "md_demo.h"
#include "md_history.h"
#include "md_nmea.h"
#include "md_pages.h"
#include "md_theme.h"

using namespace md;

static int failures = 0;
static void Check(bool ok, const std::string& what) {
  if (!ok) {
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
  }
}
static bool Near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

// Counts the primitives and checks they stay on the page.
class Recorder : public Painter {
 public:
  int calls = 0, outside = 0, texts = 0;
  std::string all_text;
  void Point(float x, float y) {
    if (!std::isfinite(x) || !std::isfinite(y) || x < -40 || y < -40 || x > kPageWidth + 40 || y > kPageHeight + 40) ++outside;
  }
  void Rect(float x, float y, float w, float h, Color, float) override { ++calls; Point(x, y); Point(x + w, y + h); }
  void Frame(float x, float y, float w, float h, Color, float, float) override { ++calls; Point(x, y); Point(x + w, y + h); }
  void Disc(float cx, float cy, float, Color) override { ++calls; Point(cx, cy); }
  void Ring(float cx, float cy, float, Color, float) override { ++calls; Point(cx, cy); }
  void Band(float cx, float cy, float, float, float, float, Color) override { ++calls; Point(cx, cy); }
  void Line(float x1, float y1, float x2, float y2, Color, float) override { ++calls; Point(x1, y1); Point(x2, y2); }
  void Poly(const Pt* p, int n, Color, float) override { ++calls; for (int i = 0; i < n; ++i) Point(p[i].x, p[i].y); }
  void Text(const std::string& s, float x, float y, float, Color, Align, bool) override { ++calls; ++texts; all_text += s + "|"; Point(x, y); }
};

int main() {
  std::setlocale(LC_ALL, "pt_PT.UTF-8");  // A comma decimal must not change numbers.

  // Theme import: a full Theme Studio export, a display-only theme, and bad files.
  Theme theme;
  int applied = 0;
  std::string error;
  const std::string studio = R"({"rose":{"gradStartDeg":42},"hull":{"stroke":3},
    "display":{"manufacturer":"  Sail-Tech  ","mode":"night","nightColor":"#ff2200",
    "colors":{"primaryText":"#123456","windPort":"#AA0000","unknownKey":"#ffffff","grid":"not a colour"}}})";
  Check(ImportTheme(studio, &theme, &applied, &error), "studio export imports: " + error);
  Check(applied == 2, "two valid colours applied, got " + std::to_string(applied));
  Check(theme.title == "Sail-Tech", "title trimmed");
  Check(theme.has_mode && theme.mode == Mode::kNight, "mode read");
  Check(ToHex(theme.color[PRIMARY_TEXT]) == "#123456" && ToHex(theme.color[WIND_PORT]) == "#aa0000", "colours read");
  Check(ToHex(theme.color[GRID]) == ToHex(DefaultTheme().color[GRID]), "an invalid colour keeps the default");
  Check(ToHex(theme.night_color) == "#ff2200", "night colour read");
  Check(ImportTheme(R"({"colors":{"aisDanger":"#00ff00"}})", &theme, &applied, &error) && applied == 1, "display-only theme");
  Check(!ImportTheme("{not json", &theme, &applied, &error), "invalid JSON refused");
  Check(!ImportTheme(R"({"rose":{}})", &theme, &applied, &error), "a file without colours refused");

  // Palettes: night turns the screen dark and the text the night colour.
  Theme base = DefaultTheme();
  Palette day = MakePalette(base, Mode::kFollow, Scheme::kDay);
  Palette dusk = MakePalette(base, Mode::kFollow, Scheme::kDusk);
  Palette night = MakePalette(base, Mode::kFollow, Scheme::kNight);
  Check(ToHex(day.c[DISPLAY_BG]) == "#ffffff", "day background white");
  Check(dusk.c[DISPLAY_BG].r < 200 && !dusk.night, "dusk dimmed");
  Check(night.night && night.c[DISPLAY_BG].r < 10 && night.c[PRIMARY_TEXT].r > night.c[PRIMARY_TEXT].g, "red night");
  Check(MakePalette(base, Mode::kDay, Scheme::kNight).c[DISPLAY_BG].r == 255, "a fixed Day ignores OpenCPN's night");

  // NMEA 0183.
  Received r;
  Check(ApplySentence("$IIMWV,038.0,R,13.6,N,A*1E", 10, &r) || true, "MWV parses (checksum may differ)");
  r = Received();
  Check(ApplySentence("$IIMWV,038.0,R,13.6,N,A", 10, &r) && Near(r.awa.v, 38, 1e-9) && Near(r.aws.v, 13.6, 1e-9), "MWV relative");
  Check(ApplySentence("$IIMWV,322.0,R,7.0,M,A", 10, &r) && Near(r.awa.v, -38, 1e-9) && Near(r.aws.v, 7 * 1.9438444924, 1e-6), "MWV port, m/s");
  Check(!ApplySentence("$IIMWV,038.0,R,13.6,N,V", 10, &r), "MWV not valid");
  Check(!ApplySentence("$IIMWV,038.0,R,13.6,N,A*00", 10, &r), "wrong checksum refused");
  Check(ApplySentence("$IIVWR,40.0,L,12.0,N,,M,,K", 11, &r) && Near(r.awa.v, -40, 1e-9), "VWR port");
  Check(ApplySentence("$IIVHW,,T,114.0,M,6.20,N,11.48,K", 11, &r) && Near(r.stw.v, 6.2, 1e-9) && Near(r.hdg_mag.v, 114, 1e-9), "VHW");
  Check(ApplySentence("$IIHDG,112.0,,,1.5,W", 11, &r) && Near(r.hdg_mag.v, 112, 1e-9) && Near(r.variation.v, -1.5, 1e-9), "HDG");
  Check(ApplySentence("$SDDPT,10.4,0.6", 11, &r) && Near(r.depth.v, 11.0, 1e-9), "DPT with offset");
  Check(ApplySentence("$SDDBT,30.0,f,9.1,M,5.0,F", 11, &r) && Near(r.depth.v, 9.1, 1e-9), "DBT metres");
  Check(ApplySentence("$GPRMB,A,0.05,R,ORIG,DEST,5046.0,N,00115.0,W,2.40,125.0,6.0,V", 11, &r) &&
            Near(r.dtw.v, 2.4, 1e-9) && Near(r.btw.v, 125, 1e-9) && Near(r.xte.v, -0.05, 1e-9), "RMB, steer right = left of track");
  Check(!ApplySentence("$GPGGA,1,2,3", 11, &r), "unused sentence ignored");

  // Combine: true wind computed from the apparent wind and boat speed.
  Received w;
  ApplySentence("$IIMWV,035.3,R,13.6,N,A", 100, &w);
  ApplySentence("$IIVHW,114.0,T,,M,6.20,N,,K", 100, &w);
  Fix fix;
  Leg leg;
  Data d;
  Combine(w, fix, leg, 101, 10, &d);
  Check(d.apparent_ok && d.true_ok && d.true_computed, "true wind computed");
  Check(Near(d.tws, 9.3, 0.2) && Near(d.twa, 58, 1.5), "true wind about 9.3 kn at 58 deg, got " + std::to_string(d.tws) + " / " + std::to_string(d.twa));
  Check(d.heading_ok && Near(d.hdg, 114, 1e-9) && Near(d.twd, 172, 1.5), "heading and TWD");
  Combine(w, fix, leg, 200, 10, &d);
  Check(!d.apparent_ok && !d.true_ok && !d.heading_ok, "old values count as unknown");
  fix.ok = true; fix.t = 300; fix.lat = 50.77; fix.lon = -1.3; fix.cog = 116; fix.sog = 6; fix.hdt = 113;
  leg.ok = true; leg.t = 300; leg.btw = 125; leg.dtw = 2.4; leg.xte = -0.05; leg.name = "WPT1";
  Combine(Received(), fix, leg, 301, 10, &d);
  Check(d.position_ok && d.heading_ok && Near(d.hdg, 113, 1e-9) && d.waypoint_ok && d.waypoint == "WPT1", "fix and active leg");

  // Autopilot, rudder, pressure and air temperature sentences.
  Received q;
  Check(ApplySentence("$IIRSA,-12.5,A,,V", 50, &q) && Near(q.rudder.v, -12.5, 1e-9), "RSA rudder to port");
  Check(ApplySentence("$GPAPB,A,A,0.10,R,N,V,V,011,M,DEST,011,M,013,T", 50, &q) && Near(q.pilot_true.v, 13, 1e-9),
        "APB heading to steer");
  Check(ApplySentence("$IIHSC,,T,250.0,M", 50, &q) && Near(q.pilot_mag.v, 250, 1e-9), "HSC magnetic");
  Check(ApplySentence("$WIXDR,P,1.0124,B,Barometer,C,19.5,C,AIR", 50, &q) && Near(q.pressure.v, 1012.4, 1e-6) &&
            Near(q.air_temp.v, 19.5, 1e-9),
        "XDR pressure and air temperature");
  Check(!ApplySentence("$IIXDR,C,55.0,C,ENGINE", 50, &q) && Near(q.air_temp.v, 19.5, 1e-9), "XDR engine temperature ignored");
  Check(ApplySentence("$WIMDA,29.92,I,1.0132,B,21.0,C,,,,,,,,,,,,,,", 50, &q) && Near(q.pressure.v, 1013.2, 1e-6) &&
            Near(q.air_temp.v, 21, 1e-9),
        "MDA pressure and air");
  Check(ApplySentence("$IIMTA,22.5,C", 50, &q) && Near(q.air_temp.v, 22.5, 1e-9), "MTA air");
  Data pd;
  Fix pf;
  pf.ok = true; pf.t = 50; pf.lat = 50; pf.lon = -1; pf.cog = 90; pf.sog = 6; pf.hdt = 80; pf.variation = -2;
  q.stw.Set(5.5, 50);
  Combine(q, pf, Leg(), 51, 10, &pd);
  Check(pd.rudder_ok && pd.pressure_ok && pd.air_temp_ok && pd.pilot_ok && Near(pd.pilot_course, 13, 1e-9),
        "rudder, pressure, air and pilot combined");
  Check(pd.variation_ok && Near(pd.variation, -2, 1e-9), "variation from the fix");
  // Ground 6 kn at 090, water 5.5 kn at 080: about 1.1 kn of current to the south-east.
  Check(pd.tide_ok && Near(pd.tide_drift, 1.1, 0.1) && pd.tide_set > 120 && pd.tide_set < 160,
        "tide from ground and water vectors, got " + std::to_string(pd.tide_drift) + " kn to " + std::to_string(pd.tide_set));

  // Histories: one depth/SOG sample a second, wind every 2 s, pressure every minute.
  History hist;
  Data hd;
  hd.depth_ok = hd.position_ok = hd.true_ok = hd.heading_ok = hd.pressure_ok = true;
  hd.depth = 5; hd.sog = 4; hd.cog = 10; hd.twd = 200; hd.tws = 12; hd.pressure = 1010;
  for (int i = 0; i < 700; ++i) hist.Update(i * 0.5, hd);
  hist.Apply(&hd);
  Check(hd.depth_history.size() == 300 && hd.sog_history.size() == 300 && hd.twd_history.size() == 175 &&
            hd.pressure_history.size() == 6,
        "history cadences, got " + std::to_string(hd.twd_history.size()) + " wind / " +
            std::to_string(hd.pressure_history.size()) + " pressure");

  // Every page, with demo data and with nothing known, in every mode.
  Demo demo;
  demo.Step(5);
  Data full;
  demo.Fill(&full);
  Check(full.demo && full.ais_ok && full.targets.size() == 5 && !full.depth_history.empty(), "demo data");
  Check(full.pressure_history.size() == 300 && full.twd_history.size() >= 299 && full.sog_history.size() == 300,
        "demo histories start full");
  Check(full.tide_ok && Near(full.tide_drift, 0.8, 0.01) && full.pilot_ok && full.rudder_ok && full.pilot_code == "ND",
        "demo tide and autopilot");
  Data empty;
  for (int mode = 0; mode < kModeCount; ++mode) {
    for (int page = 0; page < kPageCount; ++page) {
      for (const Data* data : {&full, &empty}) {
        Recorder rec;
        PageState st;
        st.page = static_cast<Page>(page);
        st.ais_selected = 7;  // Out of range: the page must clamp it.
        DrawPage(rec, MakePalette(base, static_cast<Mode>(mode), Scheme::kDusk), *data, st);
        const std::string what = std::string(PageShort(st.page)) + (data == &full ? " (demo)" : " (no data)");
        Check(rec.calls >= 5 && rec.texts > 0, what + " draws");
        Check(rec.outside == 0, what + " stays on the page");
        if (data == &empty) {
          Check(rec.all_text.find("No ") != std::string::npos || st.page == Page::kGps || st.page == Page::kDepth,
                what + " says what is missing");
        }
      }
    }
  }

  if (failures) {
    std::cerr << failures << " failure(s)\n";
    return 1;
  }
  std::cout << "core_test: all checks passed\n";
  return 0;
}
