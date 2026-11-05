#ifndef MD_SETTINGS_H_
#define MD_SETTINGS_H_

#include <string>

#include "md_pages.h"
#include "md_theme.h"

constexpr int kMaxPanes = 4;

struct MdPaneSettings {
  md::Page page = md::Page::kWindQuad;
  md::Mode mode = md::Mode::kFollow;
};

struct MdSettings {
  int pane_count = 1;
  bool visible = true;
  bool compact = false;
  bool demo = false;  // Simulated boat data, for trying the pages.
  MdPaneSettings pane[kMaxPanes] = {
      {md::Page::kWindQuad, md::Mode::kFollow}, {md::Page::kAis, md::Mode::kFollow},
      {md::Page::kSteer, md::Mode::kFollow}, {md::Page::kDepth, md::Mode::kFollow}};
};

// The theme in use: the JSON text imported from the Theme Studio (empty for the
// default theme) and its file name, kept in the plugin's own data folder.
struct MdThemeSource {
  std::string json;
  std::string file_name;
};

#endif  // MD_SETTINGS_H_
