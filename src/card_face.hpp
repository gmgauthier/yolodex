/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

#include <vector>

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
  void take_restore_point();
  bool can_restore() const;
  bool can_undo() const;
  void restore();
  bool undo();
  void apply_appearance(const std::string& family, int size_pt, int weight, int palette);

  Gtk::TextView& body_view() { return body_; }
  Gtk::Entry& index_entry() { return index_; }

  sigc::signal<void>& signal_index_changed() { return signal_index_changed_; }
  sigc::signal<void>& signal_body_changed() { return signal_body_changed_; }

 private:
  void on_index_activate();
  bool on_index_focus_out(GdkEventFocus* event);
  void on_index_edited();
  void on_body_changed();
  void ensure_tags();
  void push_undo();

  struct Snap {
    Glib::ustring index;
    Glib::ustring body;
  };

  Gtk::Frame frame_;
  Gtk::Box inner_{Gtk::ORIENTATION_VERTICAL, 0};
  Gtk::Entry index_;
  Gtk::Separator rule_{Gtk::ORIENTATION_HORIZONTAL};
  Gtk::ScrolledWindow body_scroll_;
  Gtk::TextView body_;
  Glib::RefPtr<Gtk::TextBuffer> body_buf_;
  sigc::signal<void> signal_index_changed_;
  sigc::signal<void> signal_body_changed_;
  bool suppress_ = false;
  bool undoing_ = false;
  Snap restore_;
  Snap prev_;
  std::vector<Snap> undo_;
  Glib::RefPtr<Gtk::CssProvider> chrome_css_;
};

}  // namespace yolodex
