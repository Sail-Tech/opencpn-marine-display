#ifndef MD_PI_H_
#define MD_PI_H_

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <vector>

#include <wx/timer.h>

#include "config.h"
#include "md_demo.h"
#include "md_history.h"
#include "md_nmea.h"
#include "md_settings.h"
#include "ocpn_plugin.h"

class MdPane;

// Marine Display: the Theme Studio's instrument pages in OpenCPN panes.
class MarineDisplayPi : public opencpn_plugin_118 {
 public:
  explicit MarineDisplayPi(void* plugin_manager);
  ~MarineDisplayPi() override;

  int Init() override;
  bool DeInit() override;

  int GetAPIVersionMajor() override;
  int GetAPIVersionMinor() override;
  int GetPlugInVersionMajor() override;
  int GetPlugInVersionMinor() override;
  int GetPlugInVersionPatch() override;
  int GetPlugInVersionPost() override;
  const char* GetPlugInVersionPre() override;
  const char* GetPlugInVersionBuild() override;
  wxBitmap* GetPlugInBitmap() override;
  wxString GetCommonName() override;
  wxString GetShortDescription() override;
  wxString GetLongDescription() override;

  int GetToolbarToolCount() override;
  void OnToolbarToolCallback(int id) override;
  void ShowPreferencesDialog(wxWindow* parent) override;
  void SetNMEASentence(wxString& sentence) override;
  void SetPositionFixEx(PlugIn_Position_Fix_Ex& fix) override;
  void SetActiveLegInfo(Plugin_Active_Leg_Info& leg) override;
  void SetColorScheme(PI_ColorScheme scheme) override;

 private:
  void LoadSettings();
  void SaveSettings();
  void LoadTheme();
  void SaveTheme();
  void BuildPanes();
  void RemovePanes();
  void ShowPanes(bool show);
  void Tick();
  double Now() const;

  MdSettings settings_;
  MdThemeSource theme_source_;
  md::Theme theme_;
  md::Scheme scheme_ = md::Scheme::kDay;
  md::Received received_;
  md::Fix fix_;
  md::Leg leg_;
  md::Demo demo_;
  md::Data data_;
  md::History history_;  // Of the received data (the demo keeps its own).
  std::vector<MdPane*> panes_;  // Children of OpenCPN's frame, managed by its wxAuiManager.
  wxTimer timer_;
#ifdef MARINE_DISPLAY_UI_TEST
  wxTimer test_timer_;
#endif
  int ticks_ = 0;
  int tool_id_ = -1;
  wxBitmap panel_bitmap_;
};

#endif  // MD_PI_H_
