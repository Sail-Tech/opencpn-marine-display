#ifndef MD_THEME_H_
#define MD_THEME_H_

// The display theme of the Marine Display Theme Studio: the manufacturer
// title, 27 element colours under the studio's own names, and the display
// modes (Day, Night red, Night amber, Mono). A theme exported by the studio
// (.json) is imported as it is.

#include <string>

#include "md_painter.h"

namespace md {

enum Tok {
  MANUFACTURER, CHASSIS, BEZEL, DISPLAY_BG, PRIMARY_TEXT, SECONDARY_TEXT, GRID, SEPARATOR,
  ARROWS, ARROW_SECONDARY, PORT, STARBOARD, WARNING, WIND_PORT, WIND_STARBOARD, WIND_NEUTRAL,
  WIND_FACE, WIND_TICKS, HULL, MENU, MENU_HEADER, MENU_TEXT, MENU_ACCENT,
  AIS_RINGS, AIS_TARGET, AIS_DANGER, AIS_OWN, kColorCount
};

// JSON names, exactly as in the Theme Studio's "colors" object.
extern const char* const kColorNames[kColorCount];

// Follow: Day in OpenCPN's day scheme, a dimmed Day at dusk, red Night at night.
enum class Mode { kFollow = 0, kDay, kNight, kAmber, kMono };
constexpr int kModeCount = 5;
const char* ModeName(Mode mode);   // "Follow OpenCPN", "Day", …
const char* ModeKey(Mode mode);    // "follow", "day", "night", "amber", "mono"
bool ModeFromKey(const std::string& key, Mode* mode);

enum class Scheme { kDay = 0, kDusk, kNight };

struct Theme {
  std::string title = "Marine Display";
  Color color[kColorCount];
  Color night_color = Rgb(0xff3b30);
  Color amber_color = Rgb(0xffb000);
  bool has_mode = false;  // The theme sets a display mode ("mode").
  Mode mode = Mode::kDay;
};

Theme DefaultTheme();

// Imports a Theme Studio export: {"display":{"manufacturer","mode","nightColor",
// "colors":{…}}, "rose":{…}, "hull":{…}}, or a display-only theme with "colors"
// at the top level. Unknown keys are ignored; colours it does not set keep the
// default. On success, *applied is the number of colours read.
bool ImportTheme(const std::string& json, Theme* theme, int* applied, std::string* error);

struct Palette {
  Color c[kColorCount];
  bool night = false;  // A dark mode (red or amber night).
};

// The colours a pane draws with, for its mode and OpenCPN's colour scheme.
Palette MakePalette(const Theme& theme, Mode mode, Scheme scheme);

}  // namespace md

#endif  // MD_THEME_H_
