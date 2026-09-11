/* SPDX-License-Identifier: Unlicense */

#include "main_window.hpp"
#include "about_dialog.hpp"
#include "paths.hpp"
#include "config.hpp"

#include <cmath>
#include <iostream>

#include <pango/pangocairo.h>

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

namespace yolodex {
namespace {

Gtk::Separator* toolbar_sep()
{
  auto* sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
  sep->set_margin_start(6);
  sep->set_margin_end(6);
  return sep;
}

void paint_nav_cell(Gtk::CellRenderer* cell,
                    const Gtk::TreeModel::Path& path,
                    const Gtk::TreeModel::Path& current,
                    const Gtk::TreeModel::Path& hover)
{
  if (!cell)
    return;
  const bool on = (current.size() > 0 && path.size() > 0 && path == current) ||
                  (hover.size() > 0 && path.size() > 0 && path == hover);
  if (on) {
    cell->property_cell_background() = "#C4C4BC";
    cell->property_cell_background_set() = true;
  } else {
    cell->property_cell_background_set() = false;
  }
}

bool nav_motion(Gtk::TreeView& view, Gtk::TreeModel::Path& hover, GdkEventMotion* event)
{
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn* col = nullptr;
  int cx = 0, cy = 0, bx = 0, by = 0;
  view.convert_widget_to_bin_window_coords(static_cast<int>(event->x),
                                           static_cast<int>(event->y), bx, by);
  if (view.get_path_at_pos(bx, by, path, col, cx, cy) && path.size() > 0) {
    if (hover.size() == 0 || hover != path) {
      hover = path;
      view.queue_draw();
    }
  } else if (hover.size() > 0) {
    hover.clear();
    view.queue_draw();
  }
  return false;
}

bool nav_leave(Gtk::TreeView& view, Gtk::TreeModel::Path& hover, GdkEventCrossing* event)
{
  if (event && event->detail == GDK_NOTIFY_INFERIOR)
    return false;
  if (hover.size() > 0) {
    hover.clear();
    view.queue_draw();
  }
  return false;
}

bool u_find(const Glib::ustring& hay, const Glib::ustring& needle, int from, int& out)
{
  if (needle.empty())
    return false;
  const Glib::ustring h = hay.casefold();
  const Glib::ustring n = needle.casefold();
  if (from < 0)
    from = 0;
  if (static_cast<Glib::ustring::size_type>(from) > h.size())
    return false;
  const auto pos = h.find(n, static_cast<Glib::ustring::size_type>(from));
  if (pos == Glib::ustring::npos)
    return false;
  out = static_cast<int>(pos);
  return true;
}

void draw_print_card(const Cairo::RefPtr<Cairo::Context>& cr,
                     const Glib::RefPtr<Gtk::PrintContext>& ctx, double x, double y,
                     double w, double h, const Glib::ustring& index,
                     const Glib::ustring& body)
{
  const double header = std::min(26.0, std::max(18.0, h * 0.16));
  cr->save();
  cr->rectangle(x, y, w, h);
  cr->clip();
  cr->set_source_rgb(1.0, 1.0, 1.0);
  cr->rectangle(x, y, w, h);
  cr->fill();
  cr->set_source_rgb(0.0, 0.0, 0.0);
  cr->set_line_width(1.0);
  cr->rectangle(x + 0.5, y + 0.5, w - 1.0, h - 1.0);
  cr->stroke();
  cr->move_to(x, y + header);
  cr->line_to(x + w, y + header);
  cr->stroke();

  auto layout = ctx->create_pango_layout();
  Pango::FontDescription desc;
  desc.set_family("Sans");
  desc.set_weight(Pango::WEIGHT_BOLD);
  desc.set_size(11 * Pango::SCALE);
  layout->set_font_description(desc);
  layout->set_ellipsize(Pango::ELLIPSIZE_END);
  layout->set_width(static_cast<int>((w - 16) * Pango::SCALE));
  layout->set_text(index.empty() ? Glib::ustring("Untitled") : index);
  cr->set_source_rgb(0, 0, 0);
  cr->move_to(x + 8, y + 5);
  pango_cairo_show_layout(cr->cobj(), layout->gobj());

  desc.set_family("Serif");
  desc.set_weight(Pango::WEIGHT_NORMAL);
  desc.set_size(10 * Pango::SCALE);
  layout->set_font_description(desc);
  layout->set_ellipsize(Pango::ELLIPSIZE_NONE);
  layout->set_wrap(Pango::WRAP_WORD_CHAR);
  layout->set_width(static_cast<int>((w - 16) * Pango::SCALE));
  layout->set_height(-1);
  layout->set_text(body);
  cr->save();
  cr->rectangle(x + 4, y + header + 2, w - 8, h - header - 6);
  cr->clip();
  cr->move_to(x + 8, y + header + 6);
  pango_cairo_show_layout(cr->cobj(), layout->gobj());
  cr->restore();
  cr->restore();
}

}  // namespace

MainWindow::MainWindow()
{
  settings_.load();
  set_title("YOLO-dex");
  set_default_size(settings_.window_w > 0 ? settings_.window_w : 720,
                   settings_.window_h > 0 ? settings_.window_h : 480);
  set_border_width(0);
  get_style_context()->add_class("yolodex-window");

  accel_ = Gtk::AccelGroup::create();
  add_accel_group(accel_);

  load_css();
  build_menu();
  build_toolbar();
  build_body();

  if (settings_.window_w > 0 && settings_.window_h > 0)
    resize(settings_.window_w, settings_.window_h);
  if (settings_.window_x >= 0 && settings_.window_y >= 0)
    move(settings_.window_x, settings_.window_y);
  if (settings_.paned > 40)
    paned_.set_position(settings_.paned);

  status_ctx_ = status_.get_context_id("main");
  set_status("No stack open.");

  add(root_);
  show_all();
  signal_hide().connect(sigc::mem_fun(*this, &MainWindow::persist));
  restore_session();
}

void MainWindow::load_css()
{
  const std::string css_path = find_data_file("skin/lcos/lcos.css");
  if (css_path.empty()) {
    std::cerr << "yolodex: lcos.css not found\n";
    return;
  }
  try {
    auto css = Gtk::CssProvider::create();
    css->load_from_path(css_path);
    Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(), css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  } catch (const Glib::Error& e) {
    std::cerr << "yolodex: CSS: " << e.what() << "\n";
  }
}

Gtk::MenuItem* MainWindow::add_item(Gtk::Menu& menu, const Glib::ustring& label,
                                    const sigc::slot<void()>& slot, guint key,
                                    Gdk::ModifierType mods)
{
  auto* item = Gtk::manage(new Gtk::MenuItem(label, true));
  item->signal_activate().connect(slot);
  if (key != 0)
    item->add_accelerator("activate", accel_, key, mods, Gtk::ACCEL_VISIBLE);
  menu.append(*item);
  return item;
}

void MainWindow::build_menu()
{
  auto add_menu = [this](const Glib::ustring& label, Gtk::Menu& menu) {
    auto* top = Gtk::manage(new Gtk::MenuItem(label, true));
    top->set_submenu(menu);
    menubar_.append(*top);
  };

  auto* file = Gtk::manage(new Gtk::Menu());
  add_item(*file, "_New", sigc::mem_fun(*this, &MainWindow::on_new), GDK_KEY_n,
           Gdk::CONTROL_MASK);
  add_item(*file, "_Open…", sigc::mem_fun(*this, &MainWindow::on_open), GDK_KEY_o,
           Gdk::CONTROL_MASK);
  add_item(*file, "_Save", sigc::mem_fun(*this, &MainWindow::on_save), GDK_KEY_s,
           Gdk::CONTROL_MASK);
  add_item(*file, "Save _As…", sigc::mem_fun(*this, &MainWindow::on_save_as), GDK_KEY_s,
           Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
  file->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*file, "_Print…", sigc::mem_fun(*this, &MainWindow::on_print), GDK_KEY_p,
           Gdk::CONTROL_MASK);
  add_item(*file, "Print A_ll", sigc::mem_fun(*this, &MainWindow::on_print_all));
  file->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*file, "E_xit", sigc::mem_fun(*this, &MainWindow::on_quit));
  add_menu("_File", *file);

  auto* edit = Gtk::manage(new Gtk::Menu());
  add_item(*edit, "_Undo", sigc::mem_fun(*this, &MainWindow::on_undo), GDK_KEY_z,
           Gdk::CONTROL_MASK);
  add_item(*edit, "Cu_t", sigc::mem_fun(*this, &MainWindow::on_cut));
  add_item(*edit, "_Copy", sigc::mem_fun(*this, &MainWindow::on_copy));
  add_item(*edit, "_Paste", sigc::mem_fun(*this, &MainWindow::on_paste));
  edit->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*edit, "_Restore", sigc::mem_fun(*this, &MainWindow::on_restore));
  add_menu("_Edit", *edit);

  auto* view = Gtk::manage(new Gtk::Menu());
  view_list_item_ = Gtk::manage(new Gtk::RadioMenuItem(view_group_, "_List", true));
  view_card_item_ = Gtk::manage(new Gtk::RadioMenuItem(view_group_, "_Card", true));
  view_list_item_->set_active(true);
  view_list_item_->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_view_list));
  view_card_item_->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_view_card));
  view->append(*view_list_item_);
  view->append(*view_card_item_);
  add_menu("_View", *view);

  auto* card = Gtk::manage(new Gtk::Menu());
  add_item(*card, "_Add", sigc::mem_fun(*this, &MainWindow::on_card_add));
  add_item(*card, "_Delete", sigc::mem_fun(*this, &MainWindow::on_delete_card));
  add_item(*card, "Du_plicate", sigc::mem_fun(*this, &MainWindow::on_duplicate));
  add_item(*card, "_Index…", sigc::mem_fun(*this, &MainWindow::on_index_dialog));
  card->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*card, "_Previous",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::step_card), -1));
  add_item(*card, "_Next",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::step_card), 1));
  add_menu("_Card", *card);

  auto* search = Gtk::manage(new Gtk::Menu());
  add_item(*search, "_Go To…", sigc::mem_fun(*this, &MainWindow::on_go_to), GDK_KEY_g,
           Gdk::CONTROL_MASK);
  add_item(*search, "_Find…", sigc::mem_fun(*this, &MainWindow::on_find), GDK_KEY_f,
           Gdk::CONTROL_MASK);
  add_item(*search, "Find _Next", sigc::mem_fun(*this, &MainWindow::on_find_next),
           GDK_KEY_F3, Gdk::ModifierType(0));
  add_menu("_Search", *search);

  auto* help = Gtk::manage(new Gtk::Menu());
  add_item(*help, "_About YOLO-dex", sigc::mem_fun(*this, &MainWindow::on_about));
  add_menu("_Help", *help);

  root_.pack_start(menubar_, Gtk::PACK_SHRINK);
}

void MainWindow::build_toolbar()
{
  toolbar_.set_border_width(4);
  btn_add_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_card_add));
  btn_delete_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_delete_card));
  btn_find_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_find));
  btn_print_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_print));
  btn_list_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_view_list));
  btn_card_.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_view_card));

  toolbar_.pack_start(btn_add_, Gtk::PACK_SHRINK);
  toolbar_.pack_start(btn_delete_, Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_sep(), Gtk::PACK_SHRINK);
  toolbar_.pack_start(btn_find_, Gtk::PACK_SHRINK);
  toolbar_.pack_start(btn_print_, Gtk::PACK_SHRINK);
  toolbar_.pack_start(*toolbar_sep(), Gtk::PACK_SHRINK);
  toolbar_.pack_end(btn_card_, Gtk::PACK_SHRINK);
  toolbar_.pack_end(btn_list_, Gtk::PACK_SHRINK);
  root_.pack_start(toolbar_, Gtk::PACK_SHRINK);
}

void MainWindow::build_body()
{
  list_cols_.add(col_index_);
  list_cols_.add(col_id_);
  list_store_ = Gtk::ListStore::create(list_cols_);
  list_view_.set_model(list_store_);
  list_view_.append_column("", col_index_);
  list_view_.set_headers_visible(false);
  list_view_.set_show_expanders(false);
  list_view_.set_enable_search(false);
  list_view_.get_selection()->set_mode(Gtk::SELECTION_NONE);
  list_view_.set_can_focus(true);
  list_view_.get_style_context()->add_class("yolodex-index-list");
  style_list_column();
  if (auto* col = list_view_.get_column(0)) {
    const auto cells = col->get_cells();
    if (!cells.empty()) {
      if (auto* text = dynamic_cast<Gtk::CellRendererText*>(cells[0])) {
        text->property_weight() = Pango::WEIGHT_BOLD;
        col->set_cell_data_func(*text, sigc::mem_fun(*this, &MainWindow::on_list_cell_data));
      }
    }
  }
  list_view_.add_events(Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK |
                        Gdk::BUTTON_PRESS_MASK);
  list_view_.signal_motion_notify_event().connect(
      sigc::mem_fun(*this, &MainWindow::on_list_motion), false);
  list_view_.signal_leave_notify_event().connect(
      sigc::mem_fun(*this, &MainWindow::on_list_leave), false);
  list_view_.signal_button_press_event().connect(
      sigc::mem_fun(*this, &MainWindow::on_list_button), false);
  list_view_.signal_key_press_event().connect(
      sigc::mem_fun(*this, &MainWindow::on_list_key), false);

  list_scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  list_scroll_.set_margin_start(4);
  list_scroll_.add(list_view_);
  keep_nav_left();

  card_face_.signal_index_changed().connect(
      sigc::mem_fun(*this, &MainWindow::on_index_changed));
  card_face_.signal_body_changed().connect(
      sigc::mem_fun(*this, &MainWindow::on_body_changed));

  paned_.pack1(list_scroll_, false, false);
  paned_.pack2(card_face_, true, false);
  paned_.set_position(220);

  root_.pack_start(paned_, Gtk::PACK_EXPAND_WIDGET);
  root_.pack_start(status_, Gtk::PACK_SHRINK);
}

void MainWindow::style_list_column()
{
  list_view_.set_hscroll_policy(Gtk::SCROLL_MINIMUM);
  list_view_.set_level_indentation(0);
  if (auto* col = list_view_.get_column(0)) {
    col->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    col->set_expand(true);
    const auto cells = col->get_cells();
    if (!cells.empty()) {
      if (auto* text = dynamic_cast<Gtk::CellRendererText*>(cells[0])) {
        text->property_ellipsize() = Pango::ELLIPSIZE_END;
        text->property_xalign() = 0.0;
        text->property_xpad() = 6;
      }
    }
  }
}

void MainWindow::snap_nav_left()
{
  auto snap = [](const Glib::RefPtr<Gtk::Adjustment>& adj) {
    if (!adj)
      return;
    const double lo = adj->get_lower();
    if (adj->get_value() != lo)
      adj->set_value(lo);
  };
  snap(list_scroll_.get_hadjustment());
  snap(list_view_.get_hadjustment());
}

void MainWindow::keep_nav_left()
{
  auto* v = &list_view_;
  auto* s = &list_scroll_;
  auto hook = [this, v, s]() {
    snap_nav_left();
    auto attach = [this, v, s](const Glib::RefPtr<Gtk::Adjustment>& adj) {
      if (!adj)
        return;
      adj->signal_value_changed().connect([this, v, s]() { snap_nav_left(); });
    };
    attach(s->get_hadjustment());
    attach(v->get_hadjustment());
  };
  if (list_view_.get_realized())
    hook();
  list_view_.signal_realize().connect(hook);
  list_view_.signal_map().connect([this, v, s]() {
    snap_nav_left();
    Glib::signal_idle().connect(
        [this, v, s]() {
          snap_nav_left();
          v->queue_resize();
          return false;
        },
        Glib::PRIORITY_LOW);
  });
  list_view_.signal_size_allocate().connect(
      [this, v, s](Gtk::Allocation&) { snap_nav_left(); });
  list_view_.signal_cursor_changed().connect([this, v, s]() { snap_nav_left(); });
  list_scroll_.property_hadjustment().signal_changed().connect([hook]() { hook(); });
}

void MainWindow::scroll_nav_vertically(const Gtk::TreeModel::Path& path)
{
  auto* col = list_view_.get_column(0);
  if (!col || path.empty())
    return;
  Gdk::Rectangle cell;
  list_view_.get_background_area(path, *col, cell);
  auto v = list_view_.get_vadjustment();
  if (!v)
    return;
  const double top = cell.get_y();
  const double bottom = top + cell.get_height();
  const double vis_top = v->get_value();
  const double vis_bot = vis_top + v->get_page_size();
  if (top < vis_top)
    v->set_value(top);
  else if (bottom > vis_bot)
    v->set_value(bottom - v->get_page_size());
}

void MainWindow::relayout_nav()
{
  list_view_.queue_resize();
  list_scroll_.queue_resize();
  snap_nav_left();
}

void MainWindow::set_status(const Glib::ustring& text)
{
  status_.pop(status_ctx_);
  status_.push(text, status_ctx_);
}

void MainWindow::update_title()
{
  if (!stack_.is_open()) {
    set_title("YOLO-dex");
    return;
  }
  Glib::ustring title = "YOLO-dex - ";
  title += stack_.display_name();
  if (stack_.dirty())
    title += "*";
  set_title(title);
}

void MainWindow::update_status()
{
  if (!stack_.is_open()) {
    set_status("No stack open.");
    return;
  }
  const int n = stack_.count();
  Glib::ustring text = "List — ";
  text += Glib::ustring::format(n);
  text += n == 1 ? " card" : " cards";
  if (stack_.dirty())
    text += " · modified";
  set_status(text);
}

void MainWindow::fill_list()
{
  list_hover_path_.clear();
  list_current_path_.clear();
  list_store_->clear();
  const int sel_id = stack_.selected_id();
  for (const Card& c : stack_.cards()) {
    auto it = list_store_->append();
    Glib::ustring label = c.index;
    if (label.empty())
      label = "Untitled";
    (*it)[col_index_] = label;
    (*it)[col_id_] = c.id;
    if (c.id == sel_id)
      list_current_path_ = list_store_->get_path(it);
  }
  relayout_nav();
  if (list_current_path_.size() > 0)
    scroll_nav_vertically(list_current_path_);
  list_view_.queue_draw();
}

void MainWindow::sync_list_current()
{
  list_current_path_.clear();
  const int sel_id = stack_.selected_id();
  for (auto& row : list_store_->children()) {
    if (row.get_value(col_id_) == sel_id) {
      list_current_path_ = list_store_->get_path(row);
      break;
    }
  }
  if (list_current_path_.size() > 0)
    scroll_nav_vertically(list_current_path_);
  snap_nav_left();
  list_view_.queue_draw();
}

void MainWindow::select_card_id(int id)
{
  if (id < 1)
    return;
  if (id != stack_.selected_id()) {
    flush_face();
    stack_.select_id(id);
    fill_list();
    bind_face();
    update_title();
    update_status();
    return;
  }
  sync_list_current();
}

void MainWindow::bind_face()
{
  const Card* c = stack_.selected();
  if (!c) {
    card_face_.set_enabled(false);
    return;
  }
  card_face_.set_enabled(true);
  card_face_.set_index(c->index);
  card_face_.set_body(c->body);
  card_face_.take_restore_point();
}

void MainWindow::flush_face()
{
  if (!stack_.selected())
    return;
  stack_.commit(card_face_.index(), card_face_.body());
}

void MainWindow::refresh()
{
  fill_list();
  bind_face();
  update_title();
  update_status();
}

void MainWindow::show_error(const Glib::ustring& message)
{
  Gtk::MessageDialog dlg(*this, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
  dlg.set_title("YOLO-dex");
  dlg.run();
}

bool MainWindow::confirm_discard()
{
  flush_face();
  if (!stack_.is_open() || !stack_.dirty())
    return true;

  Gtk::MessageDialog dlg(*this,
                         "Save changes to " + stack_.display_name() + "?", false,
                         Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
  dlg.set_title("YOLO-dex");
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Discard", Gtk::RESPONSE_REJECT);
  dlg.add_button("_Save", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  const int resp = dlg.run();
  if (resp == Gtk::RESPONSE_ACCEPT)
    return do_save();
  return resp == Gtk::RESPONSE_REJECT;
}

std::string MainWindow::ensure_suffix(const std::string& path) const
{
  const std::string suf = ".yolodex";
  if (path.size() >= suf.size() &&
      path.compare(path.size() - suf.size(), suf.size(), suf) == 0)
    return path;
  return path + suf;
}

std::string MainWindow::samples_dir() const
{
  const std::string p = std::string(SOURCE_ROOT) + "/data/samples";
  if (Glib::file_test(p, Glib::FILE_TEST_IS_DIR))
    return p;
  return Glib::get_home_dir();
}

bool MainWindow::do_save()
{
  flush_face();
  if (!stack_.is_open())
    return true;
  if (stack_.path().empty())
    return do_save_as();
  if (!stack_.save()) {
    show_error(stack_.error().empty() ? "Could not save." : stack_.error());
    return false;
  }
  card_face_.take_restore_point();
  update_title();
  update_status();
  return true;
}

bool MainWindow::do_save_as()
{
  flush_face();
  if (!stack_.is_open())
    return false;

  Gtk::FileChooserDialog dlg(*this, "Save Stack", Gtk::FILE_CHOOSER_ACTION_SAVE);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Save", Gtk::RESPONSE_ACCEPT);
  dlg.set_do_overwrite_confirmation(true);
  auto filter = Gtk::FileFilter::create();
  filter->set_name("YOLO-dex stack");
  filter->add_pattern("*.yolodex");
  dlg.add_filter(filter);
  auto all = Gtk::FileFilter::create();
  all->set_name("All files");
  all->add_pattern("*");
  dlg.add_filter(all);
  if (!stack_.path().empty()) {
    dlg.set_filename(stack_.path());
  } else {
    dlg.set_current_folder(Glib::get_home_dir());
    dlg.set_current_name("Untitled.yolodex");
  }
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return false;
  const std::string path = ensure_suffix(dlg.get_filename());
  dlg.hide();
  if (!stack_.save_as(path)) {
    show_error(stack_.error().empty() ? "Could not save." : stack_.error());
    return false;
  }
  card_face_.take_restore_point();
  update_title();
  update_status();
  return true;
}

void MainWindow::on_new()
{
  if (!confirm_discard())
    return;
  stack_.create_new();
  refresh();
  card_face_.focus_index();
}

void MainWindow::open_path(const std::string& path)
{
  if (!stack_.open(path)) {
    show_error(stack_.error().empty() ? "Could not open stack." : stack_.error());
    return;
  }
  last_hit_id_ = -1;
  last_query_.clear();
  refresh();
}

void MainWindow::on_open()
{
  if (!confirm_discard())
    return;
  Gtk::FileChooserDialog dlg(*this, "Open Stack", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Open", Gtk::RESPONSE_ACCEPT);
  auto filter = Gtk::FileFilter::create();
  filter->set_name("YOLO-dex stack");
  filter->add_pattern("*.yolodex");
  dlg.add_filter(filter);
  auto all = Gtk::FileFilter::create();
  all->set_name("All files");
  all->add_pattern("*");
  dlg.add_filter(all);
  dlg.set_current_folder(samples_dir());
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return;
  const std::string path = dlg.get_filename();
  dlg.hide();
  open_path(path);
}

void MainWindow::on_save()
{
  if (!stack_.is_open())
    return;
  do_save();
}

void MainWindow::on_save_as()
{
  if (!stack_.is_open())
    return;
  do_save_as();
}

void MainWindow::on_card_add()
{
  if (!stack_.is_open()) {
    on_new();
    return;
  }
  flush_face();
  stack_.add();
  refresh();
  card_face_.focus_index();
}

void MainWindow::on_delete_card()
{
  if (!stack_.selected())
    return;
  flush_face();
  Glib::ustring name = card_face_.index();
  if (name.empty())
    name = "Untitled";
  Gtk::MessageDialog dlg(*this, "Delete card \"" + name + "\"?", false,
                         Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
  dlg.set_title("YOLO-dex");
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Delete", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_CANCEL);
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return;
  stack_.remove_selected();
  refresh();
}

void MainWindow::on_duplicate()
{
  if (!stack_.selected())
    return;
  flush_face();
  stack_.duplicate_selected();
  refresh();
  card_face_.focus_index();
}

void MainWindow::on_index_dialog()
{
  Card* c = stack_.selected();
  if (!c)
    return;
  Gtk::Dialog dlg("Index", *this, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  auto* box = dlg.get_content_area();
  box->set_border_width(8);
  auto* entry = Gtk::manage(new Gtk::Entry());
  entry->set_text(card_face_.index());
  entry->set_activates_default(true);
  box->pack_start(*entry, Gtk::PACK_SHRINK);
  dlg.show_all();
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return;
  card_face_.set_index(entry->get_text());
  flush_face();
  refresh();
}

void MainWindow::persist()
{
  int x = 0, y = 0, w = 0, h = 0;
  get_position(x, y);
  get_size(w, h);
  settings_.window_x = x;
  settings_.window_y = y;
  settings_.window_w = w;
  settings_.window_h = h;
  settings_.paned = paned_.get_position();
  if (stack_.is_open() && !stack_.path().empty()) {
    settings_.last_path = stack_.path();
    settings_.last_id = stack_.selected_id();
    settings_.last_scroll = card_face_.body_scroll();
  } else {
    settings_.last_path.clear();
    settings_.last_id = -1;
    settings_.last_scroll = 0;
  }
  settings_.view = "list";
  settings_.save();
}

void MainWindow::restore_session()
{
  if (settings_.last_path.empty() ||
      !Glib::file_test(settings_.last_path, Glib::FILE_TEST_IS_REGULAR))
    return;
  if (!stack_.open(settings_.last_path)) {
    set_status("Could not restore last stack.");
    return;
  }
  if (settings_.last_id > 0)
    stack_.select_id(settings_.last_id);
  refresh();
  const double scroll = settings_.last_scroll;
  Glib::signal_idle().connect(
      [this, scroll]() {
        card_face_.set_body_scroll(scroll);
        return false;
      },
      Glib::PRIORITY_LOW);
}

void MainWindow::step_card(int delta)
{
  if (!stack_.is_open() || stack_.count() == 0)
    return;
  flush_face();
  const int n = stack_.count();
  int row = stack_.selected_row();
  if (row < 0)
    row = 0;
  row = (row + delta) % n;
  if (row < 0)
    row += n;
  stack_.select_row(row);
  fill_list();
  bind_face();
  update_title();
  update_status();
}

void MainWindow::ensure_find_dialog()
{
  if (find_dlg_)
    return;
  find_dlg_ = std::make_unique<FindDialog>(*this);
  find_dlg_->signal_find_next().connect(sigc::mem_fun(*this, &MainWindow::on_find_next));
}

void MainWindow::on_find()
{
  if (!stack_.is_open()) {
    set_status("No stack open.");
    return;
  }
  ensure_find_dialog();
  if (!last_query_.empty())
    find_dlg_->set_query(last_query_);
  find_dlg_->present_find();
}

void MainWindow::on_find_next()
{
  Glib::ustring q;
  if (find_dlg_ && find_dlg_->get_visible())
    q = find_dlg_->query();
  else
    q = last_query_;
  if (q.empty()) {
    on_find();
    return;
  }
  run_find(q, true);
}

void MainWindow::on_go_to()
{
  if (!stack_.is_open() || stack_.empty()) {
    set_status("No stack open.");
    return;
  }
  Gtk::Dialog dlg("Go To", *this, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  auto* box = dlg.get_content_area();
  box->set_border_width(8);
  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  auto* lab = Gtk::manage(new Gtk::Label("Index:"));
  auto* entry = Gtk::manage(new Gtk::Entry());
  entry->set_activates_default(true);
  entry->set_width_chars(28);
  row->pack_start(*lab, Gtk::PACK_SHRINK);
  row->pack_start(*entry, Gtk::PACK_EXPAND_WIDGET);
  box->pack_start(*row, Gtk::PACK_SHRINK);
  dlg.show_all();
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return;
  const Glib::ustring prefix = entry->get_text();
  dlg.hide();
  flush_face();
  if (!stack_.go_to_prefix(prefix)) {
    set_status("Not found.");
    return;
  }
  fill_list();
  bind_face();
  update_title();
  update_status();
}

bool MainWindow::run_find(const Glib::ustring& query, bool resume)
{
  flush_face();
  if (!stack_.is_open() || stack_.count() == 0) {
    set_status("No stack open.");
    return false;
  }
  const Glib::ustring needle = query.casefold();
  if (needle.empty())
    return false;
  const int nlen = static_cast<int>(needle.size());
  const int n = stack_.count();
  int row = stack_.selected_row();
  if (row < 0)
    row = 0;
  int field = 0;
  int off = 0;
  const bool can_resume = resume && last_hit_id_ > 0 && last_query_.casefold() == needle;
  if (can_resume) {
    row = 0;
    for (int i = 0; i < n; ++i) {
      if (stack_.cards()[static_cast<size_t>(i)].id == last_hit_id_) {
        row = i;
        break;
      }
    }
    field = last_hit_in_index_ ? 0 : 1;
    off = last_hit_offset_ + last_hit_length_;
  }
  last_query_ = query;
  const int slots = n * 2;
  const int start_slot = row * 2 + field;
  for (int i = 0; i < slots; ++i) {
    const int s = (start_slot + i) % slots;
    const int r = s / 2;
    const int f = s % 2;
    const int from = (i == 0) ? off : 0;
    const Card& c = stack_.cards()[static_cast<size_t>(r)];
    const Glib::ustring& hay = f == 0 ? c.index : c.body;
    int found = 0;
    if (!u_find(hay, query, from, found))
      continue;
    last_hit_id_ = c.id;
    last_hit_in_index_ = f == 0;
    last_hit_offset_ = found;
    last_hit_length_ = nlen;
    const bool same = c.id == stack_.selected_id();
    stack_.select_row(r);
    fill_list();
    if (!same)
      bind_face();
    card_face_.show_find_hit(f == 0, found, nlen);
    update_title();
    update_status();
    return true;
  }
  set_status("Not found.");
  return false;
}

bool MainWindow::in_editable_focus() const
{
  auto* focus = get_focus();
  if (!focus)
    return false;
  if (dynamic_cast<const Gtk::Entry*>(focus))
    return true;
  if (dynamic_cast<const Gtk::TextView*>(focus))
    return true;
  return false;
}

void MainWindow::on_quit()
{
  if (!confirm_discard())
    return;
  hide();
}

void MainWindow::on_about()
{
  AboutDialog dlg(*this);
  dlg.run();
}

void MainWindow::on_view_list()
{
  if (view_list_item_ && !view_list_item_->get_active())
    view_list_item_->set_active(true);
}

void MainWindow::on_view_card()
{
  on_not_yet("Card view");
  if (view_list_item_)
    view_list_item_->set_active(true);
}

void MainWindow::on_index_changed()
{
  if (!stack_.is_open())
    return;
  flush_face();
  fill_list();
  update_title();
  update_status();
}

void MainWindow::on_body_changed()
{
  if (!stack_.selected())
    return;
  stack_.commit(card_face_.index(), card_face_.body());
  update_title();
  update_status();
}

void MainWindow::on_list_cell_data(Gtk::CellRenderer* cell,
                                   const Gtk::TreeModel::const_iterator& it)
{
  if (!it)
    return;
  paint_nav_cell(cell, list_store_->get_path(it), list_current_path_, list_hover_path_);
}

bool MainWindow::on_list_motion(GdkEventMotion* event)
{
  return nav_motion(list_view_, list_hover_path_, event);
}

bool MainWindow::on_list_leave(GdkEventCrossing* event)
{
  return nav_leave(list_view_, list_hover_path_, event);
}

bool MainWindow::on_list_button(GdkEventButton* event)
{
  if (!event || event->button != 1 || event->type != GDK_BUTTON_PRESS)
    return false;
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn* col = nullptr;
  int cx = 0, cy = 0, bx = 0, by = 0;
  list_view_.convert_widget_to_bin_window_coords(static_cast<int>(event->x),
                                                 static_cast<int>(event->y), bx, by);
  if (!list_view_.get_path_at_pos(bx, by, path, col, cx, cy) || path.size() == 0)
    return false;
  auto it = list_store_->get_iter(path);
  if (!it)
    return false;
  select_card_id((*it)[col_id_]);
  list_view_.grab_focus();
  return true;
}

bool MainWindow::on_list_key(GdkEventKey* event)
{
  if (!event)
    return false;
  if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) {
    flush_face();
    const int row = stack_.selected_row();
    if (row > 0) {
      stack_.select_row(row - 1);
      fill_list();
      bind_face();
      update_title();
      update_status();
    } else {
      fill_list();
    }
    return true;
  }
  if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) {
    flush_face();
    const int row = stack_.selected_row();
    if (row >= 0 && row + 1 < stack_.count()) {
      stack_.select_row(row + 1);
      fill_list();
      bind_face();
      update_title();
      update_status();
    } else {
      fill_list();
    }
    return true;
  }
  if (event->keyval == GDK_KEY_Delete) {
    on_delete_card();
    return true;
  }
  return false;
}

void MainWindow::on_print()
{
  run_print(false);
}

void MainWindow::on_print_all()
{
  run_print(true);
}

void MainWindow::run_print(bool all)
{
  flush_face();
  if (!stack_.is_open() || stack_.count() == 0) {
    set_status("No stack open.");
    return;
  }
  if (!all && !stack_.selected()) {
    set_status("Nothing to print.");
    return;
  }

  struct Job {
    std::vector<Card> cards;
    bool all = false;
    int per_page = 4;
  };
  auto job = std::make_shared<Job>();
  job->all = all;
  if (all)
    job->cards = stack_.cards();
  else
    job->cards.push_back(*stack_.selected());

  auto op = Gtk::PrintOperation::create();
  op->set_job_name(stack_.display_name());
  op->set_embed_page_setup(true);
  op->signal_begin_print().connect(
      [op, job](const Glib::RefPtr<Gtk::PrintContext>&) {
        int n = 1;
        if (job->all) {
          n = static_cast<int>(
              std::ceil(static_cast<double>(job->cards.size()) / job->per_page));
          if (n < 1)
            n = 1;
        }
        op->set_n_pages(n);
      });
  op->signal_draw_page().connect(
      [job](const Glib::RefPtr<Gtk::PrintContext>& ctx, int page) {
        auto cr = ctx->get_cairo_context();
        const double pw = ctx->get_width();
        const double ph = ctx->get_height();
        const double gap = 10.0;
        const int per = job->per_page;
        const double card_h = (ph - gap * (per - 1)) / per;
        const int start = job->all ? page * per : 0;
        const int n = job->all ? per : 1;
        for (int i = 0; i < n; ++i) {
          const int idx = start + i;
          if (idx < 0 || idx >= static_cast<int>(job->cards.size()))
            break;
          const Card& c = job->cards[static_cast<size_t>(idx)];
          const double y = i * (card_h + gap);
          draw_print_card(cr, ctx, 0, y, pw, card_h, c.index, c.body);
        }
      });
  try {
    const auto result = op->run(Gtk::PRINT_OPERATION_ACTION_PRINT_DIALOG, *this);
    if (result == Gtk::PRINT_OPERATION_RESULT_APPLY)
      set_status(all ? "Sent all cards to printer." : "Sent card to printer.");
    else if (result == Gtk::PRINT_OPERATION_RESULT_ERROR)
      set_status("Print failed.");
  } catch (const Gtk::PrintError& e) {
    set_status(Glib::ustring("Print failed: ") + e.what());
  }
}

void MainWindow::on_undo()
{
  if (!card_face_.undo())
    set_status("Nothing to undo.");
}

void MainWindow::on_cut()
{
  auto* focus = get_focus();
  if (auto* e = dynamic_cast<Gtk::Entry*>(focus)) {
    e->cut_clipboard();
    return;
  }
  if (dynamic_cast<Gtk::TextView*>(focus) == &card_face_.body_view()) {
    auto clip = Gtk::Clipboard::get();
    card_face_.body_view().get_buffer()->cut_clipboard(clip);
  }
}

void MainWindow::on_copy()
{
  auto* focus = get_focus();
  if (auto* e = dynamic_cast<Gtk::Entry*>(focus)) {
    e->copy_clipboard();
    return;
  }
  if (dynamic_cast<Gtk::TextView*>(focus) == &card_face_.body_view()) {
    auto clip = Gtk::Clipboard::get();
    card_face_.body_view().get_buffer()->copy_clipboard(clip);
  }
}

void MainWindow::on_paste()
{
  auto* focus = get_focus();
  if (auto* e = dynamic_cast<Gtk::Entry*>(focus)) {
    e->paste_clipboard();
    return;
  }
  if (dynamic_cast<Gtk::TextView*>(focus) == &card_face_.body_view()) {
    auto clip = Gtk::Clipboard::get();
    card_face_.body_view().get_buffer()->paste_clipboard(clip);
  }
}

void MainWindow::on_restore()
{
  if (!stack_.selected())
    return;
  if (!card_face_.can_restore()) {
    set_status("Card is already restored.");
    return;
  }
  card_face_.restore();
}

void MainWindow::on_not_yet(const Glib::ustring& feature)
{
  set_status(feature + " — coming in a later milestone.");
}

bool MainWindow::on_key_press_event(GdkEventKey* event)
{
  if (!event)
    return Gtk::Window::on_key_press_event(event);
  const guint mods = event->state & Gtk::AccelGroup::get_default_mod_mask();
  if (event->keyval == GDK_KEY_F5) {
    if ((event->state & Gdk::SHIFT_MASK) != 0)
      on_view_card();
    else
      on_view_list();
    return true;
  }
  if (event->keyval == GDK_KEY_Insert && !in_editable_focus()) {
    on_card_add();
    return true;
  }
  if ((event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_KP_Left) &&
      mods == Gdk::MOD1_MASK) {
    step_card(-1);
    return true;
  }
  if ((event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_KP_Right) &&
      mods == Gdk::MOD1_MASK) {
    step_card(1);
    return true;
  }
  if (event->keyval == GDK_KEY_Delete) {
    auto* focus = get_focus();
    if (focus == &list_view_) {
      on_delete_card();
      return true;
    }
  }
  return Gtk::Window::on_key_press_event(event);
}

bool MainWindow::on_delete_event(GdkEventAny* event)
{
  if (!confirm_discard())
    return true;
  return Gtk::Window::on_delete_event(event);
}

}  // namespace yolodex
