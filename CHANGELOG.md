# Changelog

## 0.2.1

- Packages for more Linux systems. OpenCPN refused the Debian 12 package on Ubuntu 22.04 and Linux
  Mint 21 ("Incompatible import plugin detected"): OpenCPN 5.14 there accepts only ubuntu-x86_64 22.04
  packages. New: Ubuntu 22.04 / Mint 21 (x86_64, arm64, armhf), Ubuntu 26.04 (x86_64, arm64),
  Debian 13 x86_64 and Flatpak (OpenCPN from Flathub, runtime 25.08, x86_64 and aarch64, also for
  Raspberry Pi OS 64-bit). Ubuntu 24.04 / Mint 22 keeps using the Debian 12 package.
- No change to the plugin itself.

## 0.2.0

- Eleven more Theme Studio views: Laylines, Wind History, Wind plot, Highway, Steering, Autopilot (set
  course), Rudder, Tide, SOG / COG, Barometer (5 hours) and Weather (3 hours).
- New data: rudder angle (RSA), the autopilot's heading to steer (APB, HSC), pressure and air temperature
  (XDR, MDA, MTA), the tidal current from the ground and water vectors, magnetic variation for °M.
- Histories: SOG and COG for 5 minutes, true wind for 10 minutes, pressure for 5 hours.
- The ▦ key lists every page; the DEMO mark moved to the pane's caption.
- The demo adds a pilot in No Drift mode, a current, pressure and air temperature.

## 0.1.0

- First version: the Theme Studio's Wind quad, Steer, AIS, GPS and Depth pages in one to four dockable
  OpenCPN panes, with keys, swipe and wheel to change pages.
- Theme import from a Theme Studio export (.json) in the preferences, with the theme's name, swatches
  and the number of colours applied.
- Follow / Day / Night / Amber / Mono modes; Follow uses OpenCPN's Day, Dusk and Night colour scheme.
- Data from OpenCPN's NMEA 0183, position fix, active route and AIS targets; true wind computed when missing.
- Demo data for trying the pages.
