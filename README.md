# Marine Display for OpenCPN

The instrument pages of the [Marine Display Theme Studio](https://sail-tech.github.io/marine-display-theme-studio/web/Marine_Display_Theme_Studio.html)
in OpenCPN: one to four dockable panes, each a small instrument display with its own page and display mode.

> An aid to situational awareness. **Not for navigation.**

## Pages

| Page | Shows |
| --- | --- |
| Wind Quad | Apparent wind rose with the hull, AWS, AWA, TWS and TWA in the corners |
| Sail Steer | Heading, true wind and the bearing to the waypoint on the rose |
| Laylines | Compass ring turning with the heading, the true wind arrow and the headings on each tack (45° upwind, 150° downwind), BSPD, TWA, TWD, TWS |
| Wind History | TWD and TWS over the last 10 minutes, scaled around the present values |
| Wind plot | TWD (0–360°) and TWS over the last 10 minutes |
| Highway | Time to the waypoint, the track ahead shifted by the cross-track error, bearing, XTE and distance |
| Steering | Heading scale with the bearing to the waypoint, bearing, XTE, COG and a cross-track view |
| Autopilot | The course the pilot steers (APB/HSC) against the heading, mode badge and rudder bar |
| Rudder | Rudder angle (RSA) on the pilot's scale |
| Tide | Boat speed, heading and depth, and the current's set and drift from the ground and water vectors |
| AIS | OpenCPN's AIS targets around the boat, auto range (1.5 / 3 / 6 / 12 NM), CPA/TCPA and dangerous targets |
| GPS | Position, COG and SOG |
| SOG / COG | SOG and COG over the last 5 minutes |
| Depth | Depth and the last five minutes as a graph |
| Barometer | Pressure over the last 5 hours, air temperature, TWD and TWS |
| Weather | Pressure over the last 3 hours with large air temperature, TWD and TWS |

A quantity that is not received shows as unavailable; true wind missing from the instruments is
computed from the apparent wind, the boat speed and the heading. Directions are shown magnetic (°M)
when OpenCPN knows the variation, else true (°T). NMEA 0183 carries no autopilot mode, so the badge
shows AUTO while a heading to steer is received and STANDBY otherwise (the demo shows NO DRIFT).

## Using it

- The toolbar button shows or hides the panes. Drag a pane by its caption to dock it anywhere or float it.
- On a pane: ▲ ▼, the arrow keys, the mouse wheel or a swipe step through the pages; ▦ (or Esc) lists every
  page to go straight to one; ↵ or a tap on the AIS page selects the next target; the mode list switches
  Follow / Day / Night / Amber / Mono. With demo data the caption starts with DEMO.
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

NMEA 0183 from OpenCPN's connections (MWV, VWR, MWD, VHW, HDG, HDT, HDM, DPT, DBT, RMB, RSA, APB, HSC,
XDR, MDA, MTA), OpenCPN's position fix and active route leg, and OpenCPN's AIS targets. Nothing is sent
anywhere.

## Building

The CMake build is the OpenCPN plugin template's (see the GitHub Actions workflow). The pages, themes and
NMEA parsing in `core/` have no wxWidgets or OpenCPN dependency and their tests run on their own:

```bash
c++ -std=c++17 -Wall -Wextra -Icore tests/core_test.cpp core/*.cpp -o core_test && ./core_test
```

`tests/render_pages.cpp` draws every page with demo data to SVG files, for checking layouts without OpenCPN.

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
