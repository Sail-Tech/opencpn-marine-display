# Local setup for Marine Display.
set(OCPN_TEST_REPO "opencpn/marine-display-alpha" CACHE STRING "Development repository")
set(OCPN_BETA_REPO "opencpn/marine-display-beta" CACHE STRING "Beta repository")
set(OCPN_RELEASE_REPO "opencpn/marine-display-prod" CACHE STRING "Release repository")

set(PKG_NAME marine_display_pi)
set(PKG_VERSION 0.1.0)
set(PKG_PRERELEASE "")
set(DISPLAY_NAME MarineDisplay)
set(PLUGIN_API_NAME MarineDisplay)
set(PKG_SUMMARY "Marine Display Theme Studio instrument pages in OpenCPN panes")
set(PKG_DESCRIPTION [=[Shows the instrument pages of the Marine Display Theme Studio (wind quad, steer, AIS, GPS and depth) in one to four dockable OpenCPN panes, drawn from the NMEA 0183 data, position fix, active route and AIS targets OpenCPN receives. Themes exported from the Theme Studio (.json) are imported in the plugin's preferences; day, night, amber and mono modes follow OpenCPN's colour scheme. Includes demo data for trying the pages. An aid to situational awareness; not a navigation source.]=])
set(PKG_AUTHOR "OpenCPN Marine Display contributors")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE "https://github.com/Sail-Tech/opencpn-marine-display")
set(PKG_INFO_URL "https://github.com/Sail-Tech/opencpn-marine-display")

set(SRC
    core/md_data.h
    core/md_painter.h
    core/md_json.h
    core/md_json.cpp
    core/md_theme.h
    core/md_theme.cpp
    core/md_nmea.h
    core/md_nmea.cpp
    core/md_demo.h
    core/md_demo.cpp
    core/md_pages.h
    core/md_pages.cpp
    src/md_settings.h
    src/md_wxpainter.h
    src/md_wxpainter.cpp
    src/md_pane.h
    src/md_pane.cpp
    src/md_prefs.h
    src/md_prefs.cpp
    src/md_pi.h
    src/md_pi.cpp
)
set(PKG_API_LIB api-18)

macro(late_init)
  # The core (pages, themes, NMEA) has no wxWidgets or OpenCPN dependency.
  target_include_directories(${PACKAGE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/core" "${CMAKE_SOURCE_DIR}/src")
  if (MSVC)
    # See Online Charts: OpenCPN ships an old msvcp140.dll.
    target_compile_definitions(${PACKAGE_NAME} PRIVATE _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
    # Read the sources as UTF-8 and keep narrow literals in UTF-8.
    target_compile_options(${PACKAGE_NAME} PRIVATE /utf-8)
  endif ()
endmacro()

macro(add_plugin_libraries)
endmacro()
