/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "stack.hpp"

#include <gtkmm.h>

#include <string>
#include <vector>

namespace yolodex {

class CardView : public Gtk::DrawingArea {
 public:
  enum class Side { Before, After };

  CardView();

  void bind(const Stack& stack, Side side);
  void set_appearance(const std::string& family, int size_pt, int palette);

  sigc::signal<void, int>& signal_card_chosen() { return signal_card_chosen_; }
  sigc::signal<void, int>& signal_step() { return signal_step_; }

 protected:
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
  bool on_button_press_event(GdkEventButton* event) override;
  bool on_motion_notify_event(GdkEventMotion* event) override;
  bool on_leave_notify_event(GdkEventCrossing* event) override;
  bool on_scroll_event(GdkEventScroll* event) override;

 private:
  struct Tab {
    int id = 0;
    Glib::ustring index;
    Gdk::Rectangle rect;
  };

  void layout_tabs();
  int hit_id(double x, double y) const;
  void draw_tab(const Cairo::RefPtr<Cairo::Context>& cr, const Tab& tab, bool hover,
                bool near_face);

  std::vector<Tab> tabs_;
  int selected_id_ = -1;
  int hover_id_ = -1;
  int x_base_ = 0;
  Side side_ = Side::Before;
  std::string font_family_ = "Sans";
  int font_size_ = 11;
  int palette_ = 1;
  int tab_h_ = 22;
  int y_step_ = 18;
  sigc::signal<void, int> signal_card_chosen_;
  sigc::signal<void, int> signal_step_;
};

}  // namespace yolodex
