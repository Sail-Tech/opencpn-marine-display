#include "md_theme.h"

#include <cstring>

#include "md_json.h"

namespace md {

const char* const kColorNames[kColorCount] = {
    "manufacturer", "chassis", "bezel", "displayBackground", "primaryText", "secondaryText", "grid",
    "separator", "arrows", "arrowSecondary", "port", "starboard", "warning", "windPort",
    "windStarboard", "windNeutral", "windFace", "windTicks", "hull", "menu", "menuHeader",
    "menuText", "menuAccent", "aisRings", "aisTarget", "aisDanger", "aisOwn"};

namespace {

const uint32_t kDefaults[kColorCount] = {
    0xc5ccd0, 0x25292c, 0x0d0f10, 0xffffff, 0x0c0d0e, 0x4d5257, 0xd6d9dc,
    0xbbbbbb, 0x1f4fb5, 0xe0742c, 0xd32f2f, 0x2e9b46, 0xe0a100, 0xd9342b,
    0x2f9b4a, 0x8b9093, 0xffffff, 0xffffff, 0x8a8f93, 0x1c2023, 0x2b3135,
    0xe8ecee, 0x9be15d, 0x9aa3a8, 0x1b1d1f, 0xd32f2f, 0x1f4fb5};

const char* const kModeKeys[kModeCount] = {"follow", "day", "night", "amber", "mono"};
const char* const kModeNames[kModeCount] = {"Follow OpenCPN", "Day", "Night (red)", "Night (amber)",
                                            "Mono"};

const JsonValue* Member(const JsonValue& object, const char* key) {
  return object.IsObject() ? object.Get(key) : nullptr;
}

}  // namespace

const char* ModeName(Mode mode) { return kModeNames[static_cast<int>(mode)]; }
const char* ModeKey(Mode mode) { return kModeKeys[static_cast<int>(mode)]; }

bool ModeFromKey(const std::string& key, Mode* mode) {
  for (int i = 0; i < kModeCount; ++i) {
    if (key == kModeKeys[i]) {
      *mode = static_cast<Mode>(i);
      return true;
    }
  }
  return false;
}

Theme DefaultTheme() {
  Theme t;
  for (int i = 0; i < kColorCount; ++i) t.color[i] = Rgb(kDefaults[i]);
  return t;
}

bool ImportTheme(const std::string& json, Theme* theme, int* applied, std::string* error) {
  JsonValue root;
  if (!ParseJson(json, &root) || !root.IsObject()) {
    if (error) *error = "The file is not valid JSON.";
    return false;
  }
  const JsonValue* display = Member(root, "display");
  if (!display || !display->IsObject()) {
    const JsonValue* colors = Member(root, "colors");
    display = colors && colors->IsObject() ? &root : nullptr;
  }
  if (!display) {
    if (error) *error = "The file has no \"display\" or \"colors\" section.";
    return false;
  }
  Theme t = DefaultTheme();
  int count = 0;
  if (const JsonValue* colors = Member(*display, "colors")) {
    for (int i = 0; i < kColorCount; ++i) {
      const JsonValue* v = Member(*colors, kColorNames[i]);
      Color c;
      if (v && v->type == JsonValue::Type::kString && ParseHex(v->text, &c)) {
        t.color[i] = c;
        ++count;
      }
    }
  }
  if (const JsonValue* m = Member(*display, "manufacturer")) {
    if (m->type == JsonValue::Type::kString) {
      std::string title = m->text;
      while (!title.empty() && title.back() == ' ') title.pop_back();
      while (!title.empty() && title.front() == ' ') title.erase(0, 1);
      if (title.size() > 24) title.resize(24);
      if (!title.empty()) t.title = title;
    }
  }
  if (const JsonValue* n = Member(*display, "nightColor")) {
    Color c;
    if (n->type == JsonValue::Type::kString && ParseHex(n->text, &c)) t.night_color = c;
  }
  if (const JsonValue* m = Member(*display, "mode")) {
    Mode mode;
    if (m->type == JsonValue::Type::kString && ModeFromKey(m->text, &mode)) {
      t.has_mode = true;
      t.mode = mode;
    }
  }
  *theme = t;
  if (applied) *applied = count;
  return true;
}

Palette MakePalette(const Theme& theme, Mode mode, Scheme scheme) {
  Palette p;
  for (int i = 0; i < kColorCount; ++i) p.c[i] = theme.color[i];
  Mode effective = mode;
  double brightness = 1.0;
  if (mode == Mode::kFollow) {
    effective = scheme == Scheme::kNight ? Mode::kNight : Mode::kDay;
    if (scheme == Scheme::kDusk) brightness = 0.72;
  }
  if (effective == Mode::kNight || effective == Mode::kAmber) {
    // As the firmware's night presets: dark background, every line in one colour.
    const Color ink = effective == Mode::kNight ? theme.night_color : theme.amber_color;
    const Color lo = Dim(ink, 0.45), mid = Dim(ink, 0.7), black = Rgb(0x070909);
    for (int i = 0; i < kColorCount; ++i) p.c[i] = mid;
    p.c[DISPLAY_BG] = p.c[WIND_FACE] = black;
    p.c[PRIMARY_TEXT] = p.c[WIND_TICKS] = p.c[ARROWS] = p.c[AIS_TARGET] = p.c[AIS_DANGER] = p.c[AIS_OWN] = ink;
    p.c[MENU_TEXT] = p.c[MENU_ACCENT] = ink;
    p.c[CHASSIS] = p.c[MENU] = Rgb(0x0b0b0b);
    p.c[BEZEL] = Rgb(0x000000);
    p.c[GRID] = Dim(ink, 0.25);
    p.c[SEPARATOR] = p.c[AIS_RINGS] = lo;
    p.c[WIND_PORT] = Dim(ink, 0.9);
    p.c[WIND_STARBOARD] = Dim(ink, 0.55);
    p.c[WIND_NEUTRAL] = Dim(ink, 0.3);
    brightness *= effective == Mode::kNight ? 0.85 : 0.8;
    p.night = true;
  } else if (effective == Mode::kMono) {
    for (int i = 0; i < kColorCount; ++i) p.c[i] = Rgb(0x666666);
    p.c[DISPLAY_BG] = p.c[WIND_FACE] = p.c[WIND_TICKS] = Rgb(0xeeeeee);
    p.c[PRIMARY_TEXT] = p.c[BEZEL] = p.c[AIS_TARGET] = p.c[AIS_DANGER] = p.c[AIS_OWN] = Rgb(0x111111);
    p.c[CHASSIS] = Rgb(0x2a2a2a);
    p.c[MANUFACTURER] = Rgb(0xcccccc);
  }
  if (brightness < 1.0) {
    for (int i = 0; i < kColorCount; ++i) p.c[i] = Dim(p.c[i], brightness);
  }
  return p;
}

}  // namespace md
