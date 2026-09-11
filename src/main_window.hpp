/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "card_face.hpp"

#include <gtkmm.h>

namespace yolodex {

class MainWindow : public Gtk::Window {
 public:
  MainWindow();

 private:
  void build_menu();
  void build_toolbar();
  void build_body();
  void load_css();
  void set_status(const Glib::ustring& text);
  void update_title();
  void fill_list();
  void style_list_column();

  void on_new();
  void on_quit();
  void on_about();
  void on_view_list();
  void on_view_card();
  void on_index_changed();
  void on_not_yet(const Glib::ustring& feature);

  Gtk::MenuItem* add_item(Gtk::Menu& menu, const Glib::ustring& label,
                          const sigc::slot<void()>& slot, guint key = 0,
                          Gdk::ModifierType mods = Gdk::ModifierType(0));

 protected:
  bool on_key_press_event(GdkEventKey* event) override;

  Gtk::Box root_{Gtk::ORIENTATION_VERTICAL, 0};
  Gtk::MenuBar menubar_;
  Gtk::Box toolbar_{Gtk::ORIENTATION_HORIZONTAL, 4};
  Gtk::Button btn_add_{"Add"};
  Gtk::Button btn_delete_{"Delete"};
  Gtk::Button btn_find_{"Find"};
  Gtk::Button btn_print_{"Print"};
  Gtk::Button btn_list_{"List"};
  Gtk::Button btn_card_{"Card"};
  Gtk::RadioButtonGroup view_group_;
  Gtk::RadioMenuItem* view_list_item_ = nullptr;
  Gtk::RadioMenuItem* view_card_item_ = nullptr;
  Glib::RefPtr<Gtk::AccelGroup> accel_;
  Gtk::Paned paned_{Gtk::ORIENTATION_HORIZONTAL};
  Gtk::ScrolledWindow list_scroll_;
  Gtk::TreeView list_view_;
  CardFace card_face_;
  Gtk::Statusbar status_;
  guint status_ctx_ = 0;

  Glib::RefPtr<Gtk::ListStore> list_store_;
  Gtk::TreeModelColumn<Glib::ustring> col_index_;
  Gtk::TreeModelColumnRecord list_cols_;

  bool stack_open_ = false;
  Glib::ustring draft_index_;
};

}  // namespace yolodex
