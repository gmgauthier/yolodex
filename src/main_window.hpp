/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "card_face.hpp"
#include "find_dialog.hpp"
#include "settings.hpp"
#include "stack.hpp"

#include <gtkmm.h>

#include <memory>

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
  void update_status();
  void fill_list();
  void bind_face();
  void flush_face();
  void refresh();
  void select_card_id(int id);
  void sync_list_current();
  void style_list_column();
  void snap_nav_left();
  void keep_nav_left();
  void scroll_nav_vertically(const Gtk::TreeModel::Path& path);
  void relayout_nav();
  void show_error(const Glib::ustring& message);
  bool confirm_discard();
  bool do_save();
  bool do_save_as();
  std::string ensure_suffix(const std::string& path) const;
  std::string samples_dir() const;
  void persist();
  void restore_session();
  void open_path(const std::string& path);
  void step_card(int delta);
  void ensure_find_dialog();
  void on_find();
  void on_find_next();
  void on_go_to();
  bool run_find(const Glib::ustring& query, bool resume);
  bool in_editable_focus() const;

  void on_new();
  void on_open();
  void on_save();
  void on_save_as();
  void on_card_add();
  void on_delete_card();
  void on_duplicate();
  void on_index_dialog();
  void on_quit();
  void on_about();
  void on_view_list();
  void on_view_card();
  void on_index_changed();
  void on_body_changed();
  void on_list_cell_data(Gtk::CellRenderer* cell, const Gtk::TreeModel::const_iterator& it);
  bool on_list_motion(GdkEventMotion* event);
  bool on_list_leave(GdkEventCrossing* event);
  bool on_list_button(GdkEventButton* event);
  bool on_list_key(GdkEventKey* event);
  void on_not_yet(const Glib::ustring& feature);

  Gtk::MenuItem* add_item(Gtk::Menu& menu, const Glib::ustring& label,
                          const sigc::slot<void()>& slot, guint key = 0,
                          Gdk::ModifierType mods = Gdk::ModifierType(0));

 protected:
  bool on_key_press_event(GdkEventKey* event) override;
  bool on_delete_event(GdkEventAny* event) override;

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
  Gtk::TreeModelColumn<int> col_id_;
  Gtk::TreeModelColumnRecord list_cols_;

  Stack stack_;
  Settings settings_;
  std::unique_ptr<FindDialog> find_dlg_;
  Gtk::TreeModel::Path list_hover_path_;
  Gtk::TreeModel::Path list_current_path_;
  Glib::ustring last_query_;
  int last_hit_id_ = -1;
  bool last_hit_in_index_ = false;
  int last_hit_offset_ = 0;
  int last_hit_length_ = 0;
};

}  // namespace yolodex
