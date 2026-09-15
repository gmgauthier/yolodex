/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

namespace yolodex {

class FindDialog : public Gtk::Dialog {
 public:
  explicit FindDialog(Gtk::Window& parent);

  Glib::ustring query() const;
  void set_query(const Glib::ustring& text);
  void present_find();

  sigc::signal<void>& signal_find_next()
  {
    return signal_find_next_;
  }

 private:
  Gtk::Box box_{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::Label label_{"Find what:"};
  Gtk::Entry entry_;
  sigc::signal<void> signal_find_next_;
};

}  // namespace yolodex
