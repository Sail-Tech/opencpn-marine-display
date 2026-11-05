#include "md_pane.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <wx/menu.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "md_wxpainter.h"

namespace {

wxColour Wx(md::Color c) { return wxColour(c.r, c.g, c.b); }

}  // namespace

// A key drawn in the theme's menu colours (native buttons stay bright at night).
class MdKey : public wxWindow {
 public:
  MdKey(wxWindow* parent, const wxString& label, std::function<void()> click, int height, bool bold)
      : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
        label_(label), click_(std::move(click)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    wxFont font = GetFont();
    if (bold) font.SetWeight(wxFONTWEIGHT_BOLD);
    SetFont(font);
    SetLabelText(label);
    min_height_ = height;
    UpdateMinSize();
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { Paint(); });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) {
      pressed_ = true;
      if (!HasCapture()) CaptureMouse();
      Refresh(false);
    });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
      if (HasCapture()) ReleaseMouse();
      const bool was = pressed_;
      pressed_ = false;
      Refresh(false);
      if (was && GetClientRect().Contains(e.GetPosition()) && click_) click_();
    });
    Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) {
      pressed_ = false;
      Refresh(false);
    });
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { hover_ = true; Refresh(false); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { hover_ = false; Refresh(false); });
  }

  void SetLabelText(const wxString& label) {
    label_ = label;
    UpdateMinSize();
    Refresh(false);
  }

  void SetColours(md::Color back, md::Color text, md::Color edge, md::Color chassis) {
    if (back.r == back_.r && back.g == back_.g && back.b == back_.b && text.r == text_.r &&
        text.g == text_.g && text.b == text_.b && edge.r == edge_.r && edge.g == edge_.g &&
        edge.b == edge_.b && chassis.r == chassis_.r && chassis.g == chassis_.g && chassis.b == chassis_.b)
      return;
    back_ = back;
    text_ = text;
    edge_ = edge;
    chassis_ = chassis;
    Refresh(false);
  }

 private:
  void UpdateMinSize() {
    const wxSize text = GetTextExtent(label_);
    SetMinSize(wxSize(text.x + FromDIP(16), std::max(min_height_, text.y + FromDIP(8))));
  }

  void Paint() {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(Wx(chassis_)));
    dc.Clear();
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;
    const wxSize size = GetClientSize();
    const md::Color back = pressed_ ? md::Mix(back_, text_, 0.25) : hover_ ? md::Mix(back_, text_, 0.1) : back_;
    gc->SetBrush(wxBrush(Wx(back)));
    gc->SetPen(wxPen(Wx(edge_), 1));
    gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, FromDIP(4));
    gc->SetFont(GetFont(), Wx(text_));
    double w = 0, h = 0;
    gc->GetTextExtent(label_, &w, &h);
    gc->DrawText(label_, (size.x - w) / 2, (size.y - h) / 2);
  }

  wxString label_;
  std::function<void()> click_;
  int min_height_ = 0;
  bool pressed_ = false;
  bool hover_ = false;
  md::Color back_{60, 60, 60, 255}, text_{240, 240, 240, 255}, edge_{90, 90, 90, 255}, chassis_{30, 30, 30, 255};
};

// The instrument's screen: keeps the page's 500 x 370 proportions inside the
// window and draws it through WxPainter.
class MdScreen : public wxWindow {
 public:
  MdScreen(MdPane* pane)
      : wxWindow(pane, wxID_ANY, wxDefaultPosition, wxSize(250, 185), wxWANTS_CHARS | wxBORDER_NONE),
        pane_(pane) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(160, 118));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pane_->PaintScreen(this); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
      Refresh(false);
      e.Skip();
    });
    Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
      switch (e.GetKeyCode()) {
        case WXK_UP: pane_->Key("up"); break;
        case WXK_DOWN: pane_->Key("down"); break;
        case WXK_RETURN: case WXK_NUMPAD_ENTER: pane_->Key("enter"); break;
        case WXK_ESCAPE: pane_->Key("pages"); break;
        case 'M': case 'm': pane_->Key("menu"); break;
        default: e.Skip();
      }
    });
    Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) {
      wheel_ += e.GetWheelRotation();
      const int step = e.GetWheelDelta() > 0 ? e.GetWheelDelta() : 120;
      while (wheel_ >= step) { wheel_ -= step; pane_->Key("up"); }
      while (wheel_ <= -step) { wheel_ += step; pane_->Key("down"); }
    });
    // A swipe left or right changes the page; a tap selects the next AIS target.
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
      SetFocus();
      down_x_ = e.GetX();
      down_ = true;
      if (!HasCapture()) CaptureMouse();
    });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
      if (HasCapture()) ReleaseMouse();
      if (!down_) return;
      down_ = false;
      const int dx = e.GetX() - down_x_;
      if (std::abs(dx) > FromDIP(40)) pane_->Key(dx < 0 ? "down" : "up");
      else if (pane_->Page() == md::Page::kAis) pane_->Key("enter");
    });
    Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) { down_ = false; });
  }

 private:
  MdPane* pane_;
  int wheel_ = 0;
  int down_x_ = 0;
  bool down_ = false;
};

MdPane::MdPane(wxWindow* parent, Host host, md::Page page, md::Mode mode, bool compact)
    : wxPanel(parent, wxID_ANY), host_(std::move(host)), mode_(mode), compact_(compact) {
  state_.page = page;
  auto* main = new wxBoxSizer(wxVERTICAL);

  head_ = new wxPanel(this);
  auto* head_sizer = new wxBoxSizer(wxHORIZONTAL);
  brand_ = new wxStaticText(head_, wxID_ANY, wxEmptyString);
  wxFont brand_font = brand_->GetFont();
  brand_font.SetWeight(wxFONTWEIGHT_BOLD);
  brand_->SetFont(brand_font);
  head_sizer->Add(brand_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
  mode_key_ = new MdKey(head_, wxString(md::ModeName(mode_)) + L"  \u25BE", [this]() { ShowModeMenu(); },
                        FromDIP(26), false);
  mode_key_->SetToolTip("Display mode of this pane");
  head_sizer->Add(mode_key_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
  head_->SetSizer(head_sizer);
  main->Add(head_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);

  screen_ = new MdScreen(this);
  main->Add(screen_, 1, wxEXPAND | wxALL, 4);

  caption_ = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                              wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END);
  main->Add(caption_, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);

  keys_ = new wxPanel(this);
  auto* keys_sizer = new wxBoxSizer(wxHORIZONTAL);
  const struct {
    const wchar_t* label;
    const char* key;
    const char* tip;
  } keys[] = {{L"▦", "pages", "All pages"},
              {L"▲", "up", "Previous page"},
              {L"↵", "enter", "Enter (AIS: next target)"},
              {L"▼", "down", "Next page"},
              {L"MENU", "menu", "Display mode"}};
  for (const auto& k : keys) {
    const char* key = k.key;
    auto* button = new MdKey(keys_, k.label, [this, key]() { Key(key); }, FromDIP(30), true);
    button->SetToolTip(k.tip);
    keys_sizer->Add(button, 1, wxLEFT | wxRIGHT, 2);
    keys_list_.push_back(button);
  }
  keys_->SetSizer(keys_sizer);
  main->Add(keys_, 0, wxEXPAND | wxALL, 4);

  SetSizer(main);
  SetCompact(compact);
  ShowCaption(wxString::FromUTF8(md::PageName(state_.page)));
  Redraw();
}

void MdPane::SetPage(md::Page page) {
  state_.page = page;
  ShowCaption(wxString::FromUTF8(md::PageName(page)));
  Redraw();
}

void MdPane::SetMode(md::Mode mode) {
  mode_ = mode;
  mode_key_->SetLabelText(wxString(md::ModeName(mode)) + L"  \u25BE");
  head_->Layout();
  Redraw();
}

void MdPane::SetCompact(bool compact) {
  compact_ = compact;
  head_->Show(!compact);
  keys_->Show(!compact);
  caption_->Show(!compact);
  Layout();
}

void MdPane::ShowCaption(const wxString& text) {
  caption_text_ = text;
  UpdateCaption();
}

// Simulated data is marked in the caption, so it is never taken for the boat's own.
void MdPane::UpdateCaption() {
  const wxString text = (host_.data().demo ? wxString(L"DEMO \u00b7 ") : wxString()) + caption_text_;
  if (caption_->GetLabel() != text) caption_->SetLabel(text);
}

void MdPane::ShowModeMenu() {
  wxMenu menu;
  for (int i = 0; i < md::kModeCount; ++i) {
    menu.AppendRadioItem(wxID_HIGHEST + 1 + i, md::ModeName(static_cast<md::Mode>(i)));
  }
  menu.Check(wxID_HIGHEST + 1 + static_cast<int>(mode_), true);
  const int id = GetPopupMenuSelectionFromUser(menu, mode_key_->GetPosition() + head_->GetPosition() +
                                                         wxPoint(0, mode_key_->GetSize().y));
  if (id < wxID_HIGHEST + 1 || id >= wxID_HIGHEST + 1 + md::kModeCount) return;
  SetMode(static_cast<md::Mode>(id - wxID_HIGHEST - 1));
  if (host_.changed) host_.changed();
}

void MdPane::Key(const char* key) {
  const std::string k = key;
  const int page = static_cast<int>(state_.page);
  if (k == "up") SetPage(static_cast<md::Page>((page + md::kPageCount - 1) % md::kPageCount));
  else if (k == "down") SetPage(static_cast<md::Page>((page + 1) % md::kPageCount));
  else if (k == "enter") {
    if (state_.page == md::Page::kAis) ++state_.ais_selected;
    Redraw();
  } else if (k == "menu") {
    SetMode(static_cast<md::Mode>((static_cast<int>(mode_) + 1) % md::kModeCount));
    ShowCaption(wxString("Display: ") + md::ModeName(mode_));
  } else if (k == "pages") {
    // Every page in a list, to go straight to one.
    wxMenu menu;
    for (int i = 0; i < md::kPageCount; ++i) {
      menu.AppendRadioItem(wxID_HIGHEST + 100 + i, wxString::FromUTF8(md::PageShort(static_cast<md::Page>(i))));
    }
    menu.Check(wxID_HIGHEST + 100 + page, true);
    const int id = GetPopupMenuSelectionFromUser(menu);
    if (id < wxID_HIGHEST + 100 || id >= wxID_HIGHEST + 100 + md::kPageCount) return;
    SetPage(static_cast<md::Page>(id - wxID_HIGHEST - 100));
    if (host_.changed) host_.changed();
    return;
  }
  if (k == "up" || k == "down" || k == "menu") {
    if (host_.changed) host_.changed();
  }
}

void MdPane::Redraw() {
  const md::Palette palette = md::MakePalette(host_.theme(), mode_, host_.scheme());
  const wxColour chassis = Wx(palette.c[md::CHASSIS]), title = Wx(palette.c[md::MANUFACTURER]);
  if (GetBackgroundColour() != chassis) {
    SetBackgroundColour(chassis);
    head_->SetBackgroundColour(chassis);
    keys_->SetBackgroundColour(chassis);
    brand_->SetForegroundColour(title);
    caption_->SetForegroundColour(title);
    Refresh();
  }
  const md::Color back = palette.c[md::MENU], text_colour = palette.c[md::MENU_TEXT];
  const md::Color edge = md::Mix(back, text_colour, 0.25);
  mode_key_->SetColours(back, text_colour, edge, palette.c[md::CHASSIS]);
  for (MdKey* key : keys_list_) key->SetColours(back, text_colour, edge, palette.c[md::CHASSIS]);
  UpdateCaption();
  const wxString text = wxString::FromUTF8(host_.theme().title.c_str()).Upper();
  if (text != shown_title_) {
    brand_->SetLabel(text);
    shown_title_ = text;
    head_->Layout();
  }
  screen_->Refresh(false);
}

void MdPane::PaintScreen(wxWindow* screen) {
  wxAutoBufferedPaintDC dc(screen);
  const md::Palette palette = md::MakePalette(host_.theme(), mode_, host_.scheme());
  dc.SetBackground(wxBrush(Wx(palette.c[md::BEZEL])));
  dc.Clear();
  std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
  if (!gc) return;
  gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
  const wxSize size = screen->GetClientSize();
  const double pad = compact_ ? 0 : 4;
  const double k = std::fmin((size.x - 2 * pad) / md::kPageWidth, (size.y - 2 * pad) / md::kPageHeight);
  if (k <= 0) return;
  const double ox = (size.x - md::kPageWidth * k) / 2, oy = (size.y - md::kPageHeight * k) / 2;
  gc->Clip(ox, oy, md::kPageWidth * k, md::kPageHeight * k);
  WxPainter painter(gc.get(), ox, oy, k);
  md::DrawPage(painter, palette, host_.data(), state_);
  if (compact_ && host_.data().demo) {
    // No caption in compact panes: mark simulated data on the screen.
    painter.Rect(md::kPageWidth / 2 - 30, 0, 60, 16, palette.c[md::MENU], 3);
    painter.Text("DEMO", md::kPageWidth / 2, 13, 12, palette.c[md::WARNING], md::Align::kCenter, true);
  }
}
