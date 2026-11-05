# Marine Display for OpenCPN

The instrument pages of the [Marine Display Theme Studio](https://sail-tech.github.io/marine-display-theme-studio/web/Marine_Display_Theme_Studio.html)
in OpenCPN: one to four dockable panes, each a small instrument display with its own page and display mode.

> An aid to situational awareness. **Not for navigation.**

## Pages

| Page | Shows |
| --- | --- |
| Wind quad | Apparent wind rose with the hull, port/starboard wedges, AWS, TWS, TWA and boat speed in the corners |
| Steer | Heading, the bearing to the active waypoint, cross-track error and distance |
| AIS | OpenCPN's AIS targets around the boat, auto range (1.5 / 3 / 6 / 12 NM), CPA/TCPA and dangerous targets |
| GPS | Position, COG, SOG and the active waypoint |
| Depth | Depth and the last five minutes as a graph |

A quantity that is not received shows as unavailable; true wind missing from the instruments is
computed from the apparent wind, the boat speed and the heading.

## Using it

- The toolbar button shows or hides the panes. Drag a pane by its caption to dock it anywhere or float it.
- On a pane: the ▦ key, the arrow keys, the mouse wheel or a swipe change the page; ↵ or a tap on the AIS
  page selects the next target; the mode list switches Follow / Day / Night / Amber / Mono.
- **Follow** uses OpenCPN's colour scheme: Day, Dusk (dimmed) and Night (red).

## Preferences

*Options → Plugins → Marine Display → Preferences*

- **Instrument panes**: how many (1–4), shown or hidden, compact (the screen only) and each pane's page and mode.
- **Display theme**: *Import theme (.json)…* reads a theme exported from the Theme Studio (*Export theme*).
  Its colours, title (*manufacturer*), night colour and mode apply to every pane; the dialog shows its
  name, swatches and how many of the 27 colours it set. *Default theme* goes back to the built-in colours.
  The theme is kept in OpenCPN's data folder (`plugins/marine_display_pi/theme.json`).
- **Data**: *Demo data* runs a simulated boat with AIS targets in the Solent, for trying the pages.

## Data used

NMEA 0183 from OpenCPN's connections (MWV, VWR, MWD, VHW, HDG, HDT, HDM, DPT, DBT, RMB), OpenCPN's
position fix and active route leg, and OpenCPN's AIS targets. Nothing is sent anywhere.

## Building

The CMake build is the OpenCPN plugin template's (see the GitHub Actions workflow). The pages, themes and
NMEA parsing in `core/` have no wxWidgets or OpenCPN dependency and their tests run on their own:

```bash
c++ -std=c++17 -Wall -Wextra -Icore tests/core_test.cpp core/*.cpp -o core_test && ./core_test
```

A quick local macOS library against an installed OpenCPN:

```bash
WXDEPS=/path/to/macos_deps_universal ci/build-macos-local.sh --install
```

A local macOS package for *Options → Plugins → Import plugin…* (universal, darwin-wx32); `--install`
installs it as an import would:

```bash
WXDEPS=/path/to/macos_deps_universal DIST=dist ci/package-macos-local.sh [--install]
```

## Licence

GPL v3 or later (see `COPYING`).
