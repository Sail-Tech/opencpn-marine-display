#include "md_prefs.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/utils.h>

namespace {

// The colours shown as swatches beside the theme's name.
const md::Tok kSwatches[] = {md::DISPLAY_BG, md::PRIMARY_TEXT, md::WIND_PORT, md::WIND_STARBOARD,
                             md::ARROWS, md::ARROW_SECONDARY, md::HULL, md::AIS_DANGER, md::CHASSIS};

wxColour Wx(md::Color c) { return wxColour(c.r, c.g, c.b); }

}  // namespace

bool ShowMdPreferences(wxWindow* parent, MdSettings* settings, MdThemeSource* theme_source) {
  wxDialog dialog(parent, wxID_ANY, "Marine Display preferences", wxDefaultPosition, wxDefaultSize,
                  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  auto* main = new wxBoxSizer(wxVERTICAL);

  /* ---------- Panes ---------- */
  auto* panes_box = new wxStaticBoxSizer(wxVERTICAL, &dialog, "Instrument panes");
  wxWindow* pb = panes_box->GetStaticBox();
  auto* count_row = new wxBoxSizer(wxHORIZONTAL);
  count_row->Add(new wxStaticText(pb, wxID_ANY, "Number of panes"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
  auto* count = new wxSpinCtrl(pb, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1,
                               kMaxPanes, settings->pane_count);
  count_row->Add(count, 0, wxALIGN_CENTER_VERTICAL);
  panes_box->Add(count_row, 0, wxALL, 6);
  auto* visible = new wxCheckBox(pb, wxID_ANY, "Show the panes (also the toolbar button)");
  visible->SetValue(settings->visible);
  panes_box->Add(visible, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  auto* compact = new wxCheckBox(pb, wxID_ANY, "Compact: the screen only, without the title strip and keys");
  compact->SetValue(settings->compact);
  panes_box->Add(compact, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);

  auto* grid = new wxFlexGridSizer(3, 6, 10);
  grid->AddGrowableCol(1, 1);
  grid->AddGrowableCol(2, 1);
  wxChoice* pages[kMaxPanes];
  wxChoice* modes[kMaxPanes];
  wxStaticText* labels[kMaxPanes];
  for (int i = 0; i < kMaxPanes; ++i) {
    labels[i] = new wxStaticText(pb, wxID_ANY, wxString::Format("Pane %d", i + 1));
    pages[i] = new wxChoice(pb, wxID_ANY);
    for (int p = 0; p < md::kPageCount; ++p) pages[i]->Append(md::PageShort(static_cast<md::Page>(p)));
    pages[i]->SetSelection(static_cast<int>(settings->pane[i].page));
    modes[i] = new wxChoice(pb, wxID_ANY);
    for (int m = 0; m < md::kModeCount; ++m) modes[i]->Append(md::ModeName(static_cast<md::Mode>(m)));
    modes[i]->SetSelection(static_cast<int>(settings->pane[i].mode));
    grid->Add(labels[i], 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(pages[i], 1, wxEXPAND);
    grid->Add(modes[i], 1, wxEXPAND);
  }
  panes_box->Add(grid, 0, wxALL | wxEXPAND, 6);
  auto enable_rows = [&]() {
    for (int i = 0; i < kMaxPanes; ++i) {
      const bool on = i < count->GetValue();
      labels[i]->Enable(on);
      pages[i]->Enable(on);
      modes[i]->Enable(on);
    }
  };
  enable_rows();
  count->Bind(wxEVT_SPINCTRL, [&](wxSpinEvent&) { enable_rows(); });
  main->Add(panes_box, 0, wxALL | wxEXPAND, 8);

  /* ---------- Theme ---------- */
  MdThemeSource pending = *theme_source;
  auto* theme_box = new wxStaticBoxSizer(wxVERTICAL, &dialog, "Display theme");
  wxWindow* tb = theme_box->GetStaticBox();
  auto* name = new wxStaticText(tb, wxID_ANY, wxEmptyString);
  theme_box->Add(name, 0, wxALL | wxEXPAND, 6);
  auto* swatch_row = new wxBoxSizer(wxHORIZONTAL);
  wxPanel* swatches[sizeof(kSwatches) / sizeof(kSwatches[0])];
  for (size_t i = 0; i < sizeof(kSwatches) / sizeof(kSwatches[0]); ++i) {
    swatches[i] = new wxPanel(tb, wxID_ANY, wxDefaultPosition, dialog.FromDIP(wxSize(24, 24)), wxBORDER_SIMPLE);
    swatches[i]->SetToolTip(md::kColorNames[kSwatches[i]]);
    swatch_row->Add(swatches[i], 0, wxRIGHT, 4);
  }
  theme_box->Add(swatch_row, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  auto* buttons = new wxBoxSizer(wxHORIZONTAL);
  auto* import_button = new wxButton(tb, wxID_ANY, "Import theme (.json)...");
  auto* default_button = new wxButton(tb, wxID_ANY, "Default theme");
  buttons->Add(import_button, 0, wxRIGHT, 6);
  buttons->Add(default_button, 0);
  theme_box->Add(buttons, 0, wxALL, 6);
  auto* status = new wxStaticText(tb, wxID_ANY, wxEmptyString);
  theme_box->Add(status, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 6);
  auto* theme_note = new wxStaticText(
      tb, wxID_ANY,
      "Export a theme from the Marine Display Theme Studio (Export theme) and import the .json here.\n"
      "Its colours, night colour and title apply to every pane.");
  theme_box->Add(theme_note, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 6);

  auto show_theme = [&]() {
    md::Theme t = md::DefaultTheme();
    int applied = 0;
    std::string error;
    if (!pending.json.empty()) md::ImportTheme(pending.json, &t, &applied, &error);
    name->SetLabel(pending.json.empty()
                       ? wxString("Theme: default")
                       : wxString::Format(L"Theme: %s  ·  title “%s”",
                                          wxString::FromUTF8(pending.file_name.c_str()),
                                          wxString::FromUTF8(t.title.c_str())));
    for (size_t i = 0; i < sizeof(kSwatches) / sizeof(kSwatches[0]); ++i) {
      swatches[i]->SetBackgroundColour(Wx(t.color[kSwatches[i]]));
      swatches[i]->Refresh();
    }
  };
  show_theme();
  status->SetLabel(pending.json.empty() ? "The default colours." : "Imported earlier.");

  // Reads, checks and shows a theme file; kept only when the dialog is accepted.
  auto import_path = [&](const wxString& path) {
    wxFFile file(path, "rb");
    wxString content;
    if (!file.IsOpened() || file.Length() > 4 * 1024 * 1024 || !file.ReadAll(&content, wxConvUTF8)) {
      status->SetLabel("The file could not be read.");
      return;
    }
    const std::string json(content.utf8_str());
    md::Theme t;
    int applied = 0;
    std::string error;
    const wxString file_name = wxFileName(path).GetFullName();
    if (!md::ImportTheme(json, &t, &applied, &error)) {
      status->SetLabel(file_name + ": " + wxString::FromUTF8(error.c_str()));
      return;
    }
    pending.json = json;
    pending.file_name = std::string(file_name.utf8_str());
    show_theme();
    status->SetLabel(wxString::Format("%s: %d of %d colours applied%s. Choose OK to keep it.", file_name, applied,
                                      static_cast<int>(md::kColorCount),
                                      t.has_mode ? wxString(", mode ") + md::ModeName(t.mode) : wxString()));
    dialog.Layout();
  };
  import_button->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
    wxFileDialog open(&dialog, "Import a Theme Studio theme", wxEmptyString, wxEmptyString,
                      "Theme files (*.json)|*.json|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (open.ShowModal() == wxID_OK) import_path(open.GetPath());
  });
  default_button->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
    pending = MdThemeSource();
    show_theme();
    status->SetLabel("The default colours. Choose OK to keep them.");
  });
  main->Add(theme_box, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);

  /* ---------- Data ---------- */
  auto* data_box = new wxStaticBoxSizer(wxVERTICAL, &dialog, "Data");
  wxWindow* db = data_box->GetStaticBox();
  auto* demo = new wxCheckBox(db, wxID_ANY, "Demo data: a simulated boat and AIS targets (testing only)");
  demo->SetValue(settings->demo);
  data_box->Add(demo, 0, wxALL, 6);
  auto* data_note = new wxStaticText(
      db, wxID_ANY,
      "Without demo data the panes show what OpenCPN receives: wind, boat speed, heading and depth\n"
      "(NMEA 0183), its position fix, the active route's waypoint and its AIS targets.");
  data_box->Add(data_note, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 6);
  main->Add(data_box, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);

  main->Add(new wxStaticText(&dialog, wxID_ANY, "An aid to situational awareness. Not for navigation."), 0,
            wxLEFT | wxRIGHT, 10);
  main->Add(dialog.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 8);
  dialog.SetSizerAndFit(main);
  dialog.CentreOnParent();

#ifdef MARINE_DISPLAY_UI_TEST
  // Test builds: import $MD_TEST_THEME as the Import button would, then accept after a pause.
  wxTimer test_timer;
  int test_step = 0;
  wxString test_theme;
  if (wxGetEnv("MD_TEST_THEME", &test_theme) && !test_theme.empty()) {
    test_timer.Bind(wxEVT_TIMER, [&](wxTimerEvent&) {
      if (++test_step == 1) {
        import_path(test_theme);
      } else if (test_step == 3) {
        test_timer.Stop();
        dialog.EndModal(wxID_OK);
      }
    });
    test_timer.Start(4000);
  }
#endif
  if (dialog.ShowModal() != wxID_OK) return false;
  settings->pane_count = count->GetValue();
  settings->visible = visible->GetValue();
  settings->compact = compact->GetValue();
  settings->demo = demo->GetValue();
  for (int i = 0; i < kMaxPanes; ++i) {
    settings->pane[i].page = static_cast<md::Page>(pages[i]->GetSelection());
    settings->pane[i].mode = static_cast<md::Mode>(modes[i]->GetSelection());
  }
  *theme_source = pending;
  return true;
}
