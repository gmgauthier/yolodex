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
  ensure_tags();

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

void CardFace::ensure_tags()
{
  auto table = body_buf_->get_tag_table();
  if (table->lookup("find-hit"))
    return;
  auto tag = Gtk::TextBuffer::Tag::create("find-hit");
  tag->property_background() = "#404040";
  tag->property_foreground() = "#FFFFFF";
  table->add(tag);
}

void CardFace::clear_find_hit()
{
  if (auto t = body_buf_->get_tag_table()->lookup("find-hit"))
    body_buf_->remove_tag(t, body_buf_->begin(), body_buf_->end());
  index_.select_region(0, 0);
}

void CardFace::show_find_hit(bool in_index, int offset, int length)
{
  clear_find_hit();
  if (length <= 0 || offset < 0)
    return;
  if (in_index) {
    index_.select_region(offset, offset + length);
    index_.grab_focus();
    return;
  }
  auto a = body_buf_->get_iter_at_offset(offset);
  auto b = body_buf_->get_iter_at_offset(offset + length);
  if (auto t = body_buf_->get_tag_table()->lookup("find-hit"))
    body_buf_->apply_tag(t, a, b);
  body_.scroll_to(a, 0.2);
  body_.grab_focus();
}

double CardFace::body_scroll() const
{
  auto adj = body_scroll_.get_vadjustment();
  return adj ? adj->get_value() : 0;
}

void CardFace::set_body_scroll(double value)
{
  auto adj = body_scroll_.get_vadjustment();
  if (!adj)
    return;
  const double lo = adj->get_lower();
  const double hi = adj->get_upper() - adj->get_page_size();
  if (value < lo)
    value = lo;
  if (hi >= lo && value > hi)
    value = hi;
  adj->set_value(value);
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
