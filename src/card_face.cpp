/* SPDX-License-Identifier: Unlicense */

#include "card_face.hpp"

namespace yolodex {

CardFace::CardFace() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0)
{
  set_border_width(8);

  frame_.set_shadow_type(Gtk::SHADOW_IN);
  inner_.get_style_context()->add_class("yolodex-card");

  index_.get_style_context()->add_class("yolodex-card-index");
  index_.set_placeholder_text("Index");
  index_.signal_activate().connect(sigc::mem_fun(*this, &CardFace::on_index_activate));
  index_.signal_focus_out_event().connect(
      sigc::mem_fun(*this, &CardFace::on_index_focus_out));

  body_buf_ = Gtk::TextBuffer::create();
  body_.set_buffer(body_buf_);
  body_.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  body_.set_left_margin(16);
  body_.set_right_margin(16);
  body_.set_top_margin(12);
  body_.set_bottom_margin(16);
  body_.get_style_context()->add_class("yolodex-card-body");
  body_buf_->signal_changed().connect(sigc::mem_fun(*this, &CardFace::on_body_changed));

  body_scroll_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  body_scroll_.add(body_);

  inner_.pack_start(index_, Gtk::PACK_SHRINK);
  inner_.pack_start(body_scroll_, Gtk::PACK_EXPAND_WIDGET);
  frame_.add(inner_);
  pack_start(frame_, Gtk::PACK_EXPAND_WIDGET);

  set_enabled(false);
}

void CardFace::set_enabled(bool on)
{
  index_.set_sensitive(on);
  body_.set_sensitive(on);
  if (!on) {
    suppress_ = true;
    index_.set_text("");
    body_buf_->set_text("");
    suppress_ = false;
  }
}

void CardFace::set_index(const Glib::ustring& text)
{
  suppress_ = true;
  index_.set_text(text);
  suppress_ = false;
}

Glib::ustring CardFace::index() const
{
  return index_.get_text();
}

void CardFace::set_body(const Glib::ustring& text)
{
  suppress_ = true;
  body_buf_->set_text(text);
  suppress_ = false;
}

Glib::ustring CardFace::body() const
{
  return body_buf_->get_text();
}

void CardFace::focus_index()
{
  index_.grab_focus();
}

void CardFace::on_index_activate()
{
  if (!suppress_)
    signal_index_changed_.emit();
  body_.grab_focus();
}

bool CardFace::on_index_focus_out(GdkEventFocus*)
{
  if (!suppress_)
    signal_index_changed_.emit();
  return false;
}

void CardFace::on_body_changed()
{
  if (!suppress_)
    signal_body_changed_.emit();
}

}  // namespace yolodex
