/* SPDX-License-Identifier: Unlicense */

#include "find_dialog.hpp"

namespace yolodex {

FindDialog::FindDialog(Gtk::Window& parent)
    : Gtk::Dialog("Find", parent, false)
{
  set_modal(false);
  set_skip_taskbar_hint(true);
  set_resizable(false);
  add_button("_Close", Gtk::RESPONSE_CLOSE);
  add_button("Find _Next", Gtk::RESPONSE_ACCEPT);
  set_default_response(Gtk::RESPONSE_ACCEPT);

  auto* area = get_content_area();
  area->set_border_width(8);
  entry_.set_activates_default(true);
  entry_.set_width_chars(32);
  box_.pack_start(label_, Gtk::PACK_SHRINK);
  box_.pack_start(entry_, Gtk::PACK_EXPAND_WIDGET);
  area->pack_start(box_, Gtk::PACK_SHRINK);
  show_all_children();

  signal_response().connect([this](int resp) {
    if (resp == Gtk::RESPONSE_ACCEPT)
      signal_find_next_.emit();
    else
      hide();
  });
}

Glib::ustring FindDialog::query() const
{
  return entry_.get_text();
}

void FindDialog::set_query(const Glib::ustring& text)
{
  entry_.set_text(text);
}

void FindDialog::present_find()
{
  show();
  present();
  entry_.grab_focus();
  entry_.select_region(0, -1);
}

}  // namespace yolodex
