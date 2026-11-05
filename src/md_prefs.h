#ifndef MD_PREFS_H_
#define MD_PREFS_H_

#include "md_settings.h"

class wxWindow;

// The plugin's preferences (Options > Plugins > Marine Display > Preferences):
// the panes, their pages and modes, demo data, and the display theme, imported
// from a Theme Studio export (.json). Returns true when accepted with OK.
bool ShowMdPreferences(wxWindow* parent, MdSettings* settings, MdThemeSource* theme);

#endif  // MD_PREFS_H_
