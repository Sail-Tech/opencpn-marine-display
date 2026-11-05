#include "md_pi.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

#include <wx/aui/aui.h>
#include <wx/ffile.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/log.h>

#include "md_pane.h"
#include "md_prefs.h"

extern "C" DECL_EXP opencpn_plugin* create_pi(void* plugin_manager) {
  return new MarineDisplayPi(plugin_manager);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* plugin) { delete plugin; }

namespace {

constexpr double kMaxAge = 10.0;  // Seconds after which a value no longer counts as known.
const char* const kConfigPath = "/PlugIns/MarineDisplay";

int MinorVersion() {
  const std::string version(API_VERSION);
  const auto dot = version.find('.');
  return dot == std::string::npos ? 0 : std::atoi(version.substr(dot + 1).c_str());
}

wxString DataFile(const wxString& name) {
  wxString path = GetPluginDataDir(PKG_NAME);
  path += wxFileName::GetPathSeparator();
  path += "data";
  path += wxFileName::GetPathSeparator();
  path += name;
  return path;
}

// The imported theme is kept as a file in the plugin's own folder of OpenCPN's private data.
wxString ThemeFile() {
  wxFileName file(*GetpPrivateApplicationDataLocation(), "theme.json");
  file.AppendDir("plugins");
  file.AppendDir(PKG_NAME);
  return file.GetFullPath();
}

}  // namespace

MarineDisplayPi::MarineDisplayPi(void* plugin_manager) : opencpn_plugin_118(plugin_manager) {}
MarineDisplayPi::~MarineDisplayPi() = default;

double MarineDisplayPi::Now() const {
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}

int MarineDisplayPi::Init() {
  wxLogMessage("Marine Display: Init");
  LoadSettings();
  LoadTheme();

  const wxString icon = DataFile("marine_display_pi.svg");
  const wxString toggled = DataFile("marine_display_pi_toggled.svg");
  if (wxFileName::FileExists(icon)) {
    tool_id_ = InsertPlugInToolSVG("Marine Display", icon, icon, toggled, wxITEM_CHECK, "Marine Display",
                                   "Show or hide the Marine Display instrument panes", nullptr, -1, 0, this);
    SetToolbarItemState(tool_id_, settings_.visible);
    panel_bitmap_ = GetBitmapFromSVGFile(icon, 48, 48);
  }

  // OpenCPN has built its frame and AUI manager by now, but lay the panes out after it settles.
  GetOCPNCanvasWindow()->CallAfter([this]() { BuildPanes(); });
  timer_.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { Tick(); });
  timer_.Start(250);
#ifdef MARINE_DISPLAY_UI_TEST
  // Test builds: open the preferences once OpenCPN is up (see md_prefs.cpp).
  test_timer_.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { ShowPreferencesDialog(GetOCPNCanvasWindow()); });
  test_timer_.StartOnce(30000);
#endif

  return WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL | WANTS_PREFERENCES | WANTS_NMEA_SENTENCES |
         WANTS_NMEA_EVENTS;
}

bool MarineDisplayPi::DeInit() {
  wxLogMessage("Marine Display: DeInit");
  timer_.Stop();
  RemovePanes();
  if (tool_id_ >= 0) {
    RemovePlugInTool(tool_id_);
    tool_id_ = -1;
  }
  return true;
}

int MarineDisplayPi::GetAPIVersionMajor() { return std::atoi(API_VERSION); }
int MarineDisplayPi::GetAPIVersionMinor() { return MinorVersion(); }
int MarineDisplayPi::GetPlugInVersionMajor() { return PLUGIN_VERSION_MAJOR; }
int MarineDisplayPi::GetPlugInVersionMinor() { return PLUGIN_VERSION_MINOR; }
int MarineDisplayPi::GetPlugInVersionPatch() { return PLUGIN_VERSION_PATCH; }
int MarineDisplayPi::GetPlugInVersionPost() { return PLUGIN_VERSION_TWEAK; }
const char* MarineDisplayPi::GetPlugInVersionPre() { return PKG_PRERELEASE; }
const char* MarineDisplayPi::GetPlugInVersionBuild() { return PKG_BUILD_INFO; }
wxBitmap* MarineDisplayPi::GetPlugInBitmap() { return &panel_bitmap_; }
wxString MarineDisplayPi::GetCommonName() { return PLUGIN_API_NAME; }
wxString MarineDisplayPi::GetShortDescription() { return PKG_SUMMARY; }
wxString MarineDisplayPi::GetLongDescription() { return PKG_DESCRIPTION; }
int MarineDisplayPi::GetToolbarToolCount() { return 1; }

/* ---------- Settings ---------- */

void MarineDisplayPi::LoadSettings() {
  wxFileConfig* config = GetOCPNConfigObject();
  if (!config) return;
  const wxString previous = config->GetPath();
  config->SetPath(kConfigPath);
  config->Read("PaneCount", &settings_.pane_count, 1);
  settings_.pane_count = std::max(1, std::min(kMaxPanes, settings_.pane_count));
  config->Read("Visible", &settings_.visible, true);
  config->Read("Compact", &settings_.compact, false);
  config->Read("Demo", &settings_.demo, false);
  for (int i = 0; i < kMaxPanes; ++i) {
    wxString page, mode;
    if (config->Read(wxString::Format("Pane%dPage", i + 1), &page)) {
      md::Page p;
      if (md::PageFromKey(page.utf8_str(), &p)) settings_.pane[i].page = p;
    }
    if (config->Read(wxString::Format("Pane%dMode", i + 1), &mode)) {
      md::Mode m;
      if (md::ModeFromKey(std::string(mode.utf8_str()), &m)) settings_.pane[i].mode = m;
    }
  }
  wxString theme_name;
  config->Read("ThemeFile", &theme_name);
  theme_source_.file_name = std::string(theme_name.utf8_str());
  config->SetPath(previous);
}

void MarineDisplayPi::SaveSettings() {
  wxFileConfig* config = GetOCPNConfigObject();
  if (!config) return;
  // The panes hold the pages and modes changed with their keys.
  for (size_t i = 0; i < panes_.size(); ++i) {
    settings_.pane[i].page = panes_[i]->Page();
    settings_.pane[i].mode = panes_[i]->Mode();
  }
  const wxString previous = config->GetPath();
  config->SetPath(kConfigPath);
  config->Write("PaneCount", settings_.pane_count);
  config->Write("Visible", settings_.visible);
  config->Write("Compact", settings_.compact);
  config->Write("Demo", settings_.demo);
  for (int i = 0; i < kMaxPanes; ++i) {
    config->Write(wxString::Format("Pane%dPage", i + 1), md::PageKey(settings_.pane[i].page));
    config->Write(wxString::Format("Pane%dMode", i + 1), md::ModeKey(settings_.pane[i].mode));
  }
  config->Write("ThemeFile", wxString::FromUTF8(theme_source_.file_name.c_str()));
  config->Flush();
  config->SetPath(previous);
}

void MarineDisplayPi::LoadTheme() {
  theme_ = md::DefaultTheme();
  theme_source_.json.clear();
  if (!wxFileName::FileExists(ThemeFile())) return;
  wxFFile file(ThemeFile(), "rb");
  wxString content;
  if (!file.IsOpened() || !file.ReadAll(&content, wxConvUTF8)) return;
  const std::string json(content.utf8_str());
  md::Theme theme;
  int applied = 0;
  std::string error;
  if (md::ImportTheme(json, &theme, &applied, &error)) {
    theme_ = theme;
    theme_source_.json = json;
    wxLogMessage("Marine Display: theme %s, %d colours", wxString::FromUTF8(theme_source_.file_name.c_str()), applied);
  } else {
    wxLogWarning("Marine Display: the saved theme could not be read: %s", wxString::FromUTF8(error.c_str()));
  }
}

void MarineDisplayPi::SaveTheme() {
  const wxString path = ThemeFile();
  if (theme_source_.json.empty()) {
    if (wxFileName::FileExists(path)) wxRemoveFile(path);
    theme_source_.file_name.clear();
    return;
  }
  wxFileName::Mkdir(wxFileName(path).GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
  wxFFile file(path, "wb");
  if (!file.IsOpened() || !file.Write(theme_source_.json.data(), theme_source_.json.size())) {
    wxLogWarning("Marine Display: the theme could not be saved to %s", path);
  }
}

/* ---------- Panes ---------- */

void MarineDisplayPi::BuildPanes() {
  RemovePanes();
  wxAuiManager* aui = GetFrameAuiManager();
  if (!aui || !aui->GetManagedWindow()) return;
  MdPane::Host host;
  host.data = [this]() -> const md::Data& { return data_; };
  host.theme = [this]() -> const md::Theme& { return theme_; };
  host.scheme = [this]() { return scheme_; };
  host.changed = [this]() { SaveSettings(); };
  for (int i = 0; i < settings_.pane_count; ++i) {
    auto* pane = new MdPane(aui->GetManagedWindow(), host, settings_.pane[i].page, settings_.pane[i].mode,
                            settings_.compact);
    wxAuiPaneInfo info;
    info.Name(wxString::Format("MarineDisplay%d", i + 1))
        .Caption("Marine Display")
        .CaptionVisible(true)
        .Right()
        .Layer(1)
        // Up to two panes in a column; three or four make two columns, so the screens keep their size.
        .Row(settings_.pane_count > 2 ? i / 2 : 0)
        .Position(settings_.pane_count > 2 ? i % 2 : i)
        .BestSize(pane->FromDIP(wxSize(340, settings_.compact ? 260 : 340)))
        .MinSize(pane->FromDIP(wxSize(180, 140)))
        .FloatingSize(pane->FromDIP(wxSize(360, 360)))
        .Dockable(true)
        .Floatable(true)
        .CloseButton(false)
        .Show(settings_.visible);
    aui->AddPane(pane, info);
    panes_.push_back(pane);
  }
  aui->Update();
}

void MarineDisplayPi::RemovePanes() {
  wxAuiManager* aui = GetFrameAuiManager();
  for (MdPane* pane : panes_) {
    if (aui) aui->DetachPane(pane);
    pane->Destroy();
  }
  panes_.clear();
  if (aui) aui->Update();
}

void MarineDisplayPi::ShowPanes(bool show) {
  settings_.visible = show;
  wxAuiManager* aui = GetFrameAuiManager();
  if (aui) {
    for (MdPane* pane : panes_) aui->GetPane(pane).Show(show);
    aui->Update();
  }
  if (tool_id_ >= 0) SetToolbarItemState(tool_id_, show);
  SaveSettings();
}

void MarineDisplayPi::OnToolbarToolCallback(int id) {
  if (id == tool_id_) ShowPanes(!settings_.visible);
}

void MarineDisplayPi::ShowPreferencesDialog(wxWindow* parent) {
  SaveSettings();  // Takes the pages and modes the panes show now.
  MdSettings settings = settings_;
  MdThemeSource theme = theme_source_;
  if (!ShowMdPreferences(parent, &settings, &theme)) return;
  const bool rebuild = settings.pane_count != settings_.pane_count || settings.compact != settings_.compact;
  settings_ = settings;
  theme_source_ = theme;
  SaveTheme();
  LoadTheme();
  if (theme_.has_mode) {
    // A theme that sets a mode applies it to every pane.
    for (int i = 0; i < kMaxPanes; ++i) settings_.pane[i].mode = theme_.mode;
  }
  if (rebuild) {
    BuildPanes();
  } else {
    for (size_t i = 0; i < panes_.size(); ++i) {
      panes_[i]->SetPage(settings_.pane[i].page);
      panes_[i]->SetMode(settings_.pane[i].mode);
    }
  }
  ShowPanes(settings_.visible);
  for (MdPane* pane : panes_) pane->Redraw();
}

/* ---------- Data ---------- */

void MarineDisplayPi::SetNMEASentence(wxString& sentence) {
  md::ApplySentence(std::string(sentence.mb_str(wxConvUTF8)), Now(), &received_);
}

void MarineDisplayPi::SetPositionFixEx(PlugIn_Position_Fix_Ex& p) {
  // OpenCPN keeps reporting its last own-ship position without a GPS; only a
  // live fix (a speed or satellites, recent) counts as a position.
  const bool live = (std::isfinite(p.Sog) || p.nSats > 0) && p.FixTime > 0 &&
                    std::llabs(static_cast<long long>(std::time(nullptr) - p.FixTime)) < 60;
  fix_.ok = live && std::isfinite(p.Lat) && std::isfinite(p.Lon);
  fix_.t = Now();
  fix_.lat = p.Lat;
  fix_.lon = p.Lon;
  fix_.cog = p.Cog;
  fix_.sog = p.Sog;
  fix_.variation = p.Var;
  fix_.hdm = p.Hdm;
  fix_.hdt = p.Hdt;
}

void MarineDisplayPi::SetActiveLegInfo(Plugin_Active_Leg_Info& leg) {
  leg_.ok = true;
  leg_.t = Now();
  leg_.btw = leg.Btw;
  leg_.dtw = leg.Dtw;
  leg_.xte = leg.Xte;
  leg_.name = std::string(leg.wp_name.utf8_str());
}

void MarineDisplayPi::SetColorScheme(PI_ColorScheme scheme) {
  scheme_ = scheme == PI_GLOBAL_COLOR_SCHEME_NIGHT  ? md::Scheme::kNight
            : scheme == PI_GLOBAL_COLOR_SCHEME_DUSK ? md::Scheme::kDusk
                                                    : md::Scheme::kDay;
  for (MdPane* pane : panes_) pane->Redraw();
}

void MarineDisplayPi::Tick() {
  ++ticks_;
  const bool second = ticks_ % 4 == 0;
  if (settings_.demo) {
    demo_.Step(0.25);
    demo_.Fill(&data_);
  } else {
    md::Combine(received_, fix_, leg_, Now(), kMaxAge, &data_);
    data_.demo = false;
    if (second) {
      // AIS targets with OpenCPN's own range, bearing, CPA and TCPA.
      data_.targets.clear();
      if (ArrayOfPlugIn_AIS_Targets* targets = GetAISTargetArray()) {
        for (size_t i = 0; i < targets->GetCount(); ++i) {
          const PlugIn_AIS_Target* t = targets->Item(i);
          md::AisTarget a;
          a.mmsi = static_cast<uint32_t>(t->MMSI);
          a.name = t->ShipName;
          while (!a.name.empty() && (a.name.back() == ' ' || a.name.back() == '@')) a.name.pop_back();
          a.brg = t->Brg;
          a.range = t->Range_NM;
          a.cog = t->COG;
          a.sog = t->SOG;
          a.cpa_valid = t->bCPA_Valid;
          a.cpa = t->CPA;
          a.tcpa = t->TCPA;
          a.danger = t->alarm_state != PI_AIS_NO_ALARM;
          if (std::isfinite(a.range) && std::isfinite(a.brg)) data_.targets.push_back(a);
          delete t;
        }
        delete targets;
      }
    }
    history_.Update(Now(), data_);
    history_.Apply(&data_);
    data_.ais_ok = true;
  }
  for (MdPane* pane : panes_) {
    if (pane->IsShownOnScreen()) pane->Redraw();
  }
}
