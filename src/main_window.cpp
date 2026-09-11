/* SPDX-License-Identifier: Unlicense */

#include "main_window.hpp"
#include "about_dialog.hpp"
#include "paths.hpp"
#include "config.hpp"

#include <iostream>

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

}  // namespace

MainWindow::MainWindow()
{
  set_title("YOLO-dex");
  set_default_size(720, 480);
  set_border_width(0);
  get_style_context()->add_class("yolodex-window");

  accel_ = Gtk::AccelGroup::create();
  add_accel_group(accel_);

  load_css();
  build_menu();
  build_toolbar();
  build_body();

  status_ctx_ = status_.get_context_id("main");
  set_status("No stack open.");

  add(root_);
  show_all();
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
  add_item(*file, "_Open…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Open")),
           GDK_KEY_o, Gdk::CONTROL_MASK);
  add_item(*file, "_Save",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Save")),
           GDK_KEY_s, Gdk::CONTROL_MASK);
  add_item(*file, "Save _As…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Save As")),
           GDK_KEY_s, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
  file->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*file, "_Print…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Print")),
           GDK_KEY_p, Gdk::CONTROL_MASK);
  add_item(*file, "Print A_ll",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Print All")));
  file->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*file, "E_xit", sigc::mem_fun(*this, &MainWindow::on_quit));
  add_menu("_File", *file);

  auto* edit = Gtk::manage(new Gtk::Menu());
  add_item(*edit, "_Undo",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Undo")));
  add_item(*edit, "Cu_t",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Cut")));
  add_item(*edit, "_Copy",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Copy")));
  add_item(*edit, "_Paste",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Paste")));
  edit->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*edit, "_Restore",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Restore")));
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
  add_item(*card, "_Add",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Add")));
  add_item(*card, "_Delete",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Delete")));
  add_item(*card, "Du_plicate",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Duplicate")));
  add_item(*card, "_Index…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Index")));
  add_menu("_Card", *card);

  auto* search = Gtk::manage(new Gtk::Menu());
  add_item(*search, "_Go To…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Go To")),
           GDK_KEY_g, Gdk::CONTROL_MASK);
  add_item(*search, "_Find…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Find")),
           GDK_KEY_f, Gdk::CONTROL_MASK);
  add_item(*search, "Find _Next",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet),
                      Glib::ustring("Find Next")),
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
  btn_add_.signal_clicked().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Add")));
  btn_delete_.signal_clicked().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Delete")));
  btn_find_.signal_clicked().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Find")));
  btn_print_.signal_clicked().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Print")));
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
  list_store_ = Gtk::ListStore::create(list_cols_);
  list_view_.set_model(list_store_);
  list_view_.set_headers_visible(false);
  list_view_.set_enable_search(false);
  list_view_.get_style_context()->add_class("yolodex-index-list");
  style_list_column();

  list_scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  list_scroll_.add(list_view_);

  card_face_.signal_index_changed().connect(
      sigc::mem_fun(*this, &MainWindow::on_index_changed));

  paned_.pack1(list_scroll_, false, false);
  paned_.pack2(card_face_, true, false);
  paned_.set_position(220);

  root_.pack_start(paned_, Gtk::PACK_EXPAND_WIDGET);
  root_.pack_start(status_, Gtk::PACK_SHRINK);
}

void MainWindow::style_list_column()
{
  auto* cell = Gtk::manage(new Gtk::CellRendererText());
  cell->property_ellipsize() = Pango::ELLIPSIZE_END;
  auto* col = Gtk::manage(new Gtk::TreeViewColumn("", *cell));
  col->add_attribute(cell->property_text(), col_index_);
  col->set_expand(true);
  list_view_.append_column(*col);
}

void MainWindow::set_status(const Glib::ustring& text)
{
  status_.pop(status_ctx_);
  status_.push(text, status_ctx_);
}

void MainWindow::update_title()
{
  if (!stack_open_) {
    set_title("YOLO-dex");
    return;
  }
  set_title("YOLO-dex - Untitled");
}

void MainWindow::fill_list()
{
  list_store_->clear();
  if (!stack_open_)
    return;
  auto row = *list_store_->append();
  Glib::ustring label = draft_index_;
  if (label.empty())
    label = "Untitled";
  row[col_index_] = label;
}

void MainWindow::on_new()
{
  stack_open_ = true;
  draft_index_.clear();
  card_face_.set_enabled(true);
  card_face_.set_index("");
  card_face_.set_body("");
  fill_list();
  update_title();
  set_status("Untitled — 1 card");
  card_face_.focus_index();
}

void MainWindow::on_quit()
{
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
  if (!stack_open_)
    return;
  draft_index_ = card_face_.index();
  fill_list();
}

void MainWindow::on_not_yet(const Glib::ustring& feature)
{
  set_status(feature + " — coming in a later milestone.");
}

bool MainWindow::on_key_press_event(GdkEventKey* event)
{
  if (event && event->keyval == GDK_KEY_F5) {
    if ((event->state & Gdk::SHIFT_MASK) != 0)
      on_view_card();
    else
      on_view_list();
    return true;
  }
  return Gtk::Window::on_key_press_event(event);
}

}  // namespace yolodex
