#ifndef MD_PANE_H_
#define MD_PANE_H_

#include <functional>
#include <vector>

#include <wx/panel.h>

#include "md_data.h"
#include "md_pages.h"
#include "md_theme.h"

class wxStaticText;
class MdScreen;
class MdKey;

// One instrument pane, docked in OpenCPN's window (wxAUI) or floating: the
// title strip with the display mode, the screen and the five keys of the
// Theme Studio's instrument. Keys, the arrow keys, Enter, Esc and M, the mouse
// wheel and a swipe on the screen change the page; Enter or a tap selects the
// next AIS target.
class MdPane : public wxPanel {
 public:
  struct Host {
    std::function<const md::Data&()> data;
    std::function<const md::Theme&()> theme;
    std::function<md::Scheme()> scheme;
    std::function<void()> changed;  // The page or mode changed: save the settings.
  };

  MdPane(wxWindow* parent, Host host, md::Page page, md::Mode mode, bool compact);

  md::Page Page() const { return state_.page; }
  md::Mode Mode() const { return mode_; }
  void SetPage(md::Page page);
  void SetMode(md::Mode mode);
  void SetCompact(bool compact);
  // Redraws the screen and takes the theme's chassis colours.
  void Redraw();
  void Key(const char* key);

  // Used by the screen.
  void PaintScreen(wxWindow* screen);

 private:
  void ShowCaption(const wxString& text);
  void UpdateCaption();
  void ShowModeMenu();

  Host host_;
  md::PageState state_;
  md::Mode mode_;
  bool compact_ = false;
  wxPanel* head_ = nullptr;
  wxStaticText* brand_ = nullptr;
  MdKey* mode_key_ = nullptr;  // The mode, in the header; a click opens the list of modes.
  std::vector<MdKey*> keys_list_;
  MdScreen* screen_ = nullptr;
  wxStaticText* caption_ = nullptr;
  wxString caption_text_;  // Without the DEMO mark.
  wxPanel* keys_ = nullptr;
  wxString shown_title_;
};

#endif  // MD_PANE_H_
