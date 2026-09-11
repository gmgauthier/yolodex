/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

namespace yolodex {

class CardFace : public Gtk::Box {
 public:
  CardFace();

  void set_enabled(bool on);
  void set_index(const Glib::ustring& text);
  Glib::ustring index() const;
  void set_body(const Glib::ustring& text);
  Glib::ustring body() const;
  void focus_index();
  void show_find_hit(bool in_index, int offset, int length);
  void clear_find_hit();
  double body_scroll() const;
  void set_body_scroll(double value);

  Gtk::TextView& body_view() { return body_; }

  sigc::signal<void>& signal_index_changed() { return signal_index_changed_; }
  sigc::signal<void>& signal_body_changed() { return signal_body_changed_; }

 private:
  void on_index_activate();
  bool on_index_focus_out(GdkEventFocus* event);
  void on_body_changed();
  void ensure_tags();

  Gtk::Frame frame_;
  Gtk::Box inner_{Gtk::ORIENTATION_VERTICAL, 0};
  Gtk::Entry index_;
  Gtk::ScrolledWindow body_scroll_;
  Gtk::TextView body_;
  Glib::RefPtr<Gtk::TextBuffer> body_buf_;
  sigc::signal<void> signal_index_changed_;
  sigc::signal<void> signal_body_changed_;
  bool suppress_ = false;
};

}  // namespace yolodex
