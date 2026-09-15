/* SPDX-License-Identifier: Unlicense */

#include "card_face.hpp"

#include <algorithm>
#include <cstdio>

namespace yolodex {

CardFace::CardFace()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0)
{
  set_border_width(8);

  frame_.set_shadow_type(Gtk::SHADOW_IN);
  inner_.get_style_context()->add_class("yolodex-card");

  index_.get_style_context()->add_class("yolodex-card-index");
  index_.set_placeholder_text("Index");
  index_.set_margin_start(2);
  index_.set_margin_end(2);
  index_.signal_activate().connect(sigc::mem_fun(*this, &CardFace::on_index_activate));
  index_.signal_focus_out_event().connect(sigc::mem_fun(*this, &CardFace::on_index_focus_out));
  index_.signal_changed().connect(sigc::mem_fun(*this, &CardFace::on_index_edited));

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
  inner_.pack_start(rule_, Gtk::PACK_SHRINK);
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

void CardFace::on_index_edited()
{
  if (suppress_ || undoing_)
    return;
  push_undo();
  prev_.index = index_.get_text();
}

void CardFace::on_body_changed()
{
  if (suppress_)
    return;
  if (!undoing_) {
    push_undo();
    prev_.index = index();
    prev_.body = body();
  }
  signal_body_changed_.emit();
}

void CardFace::push_undo()
{
  if (!undo_.empty() && undo_.back().index == prev_.index && undo_.back().body == prev_.body)
    return;
  undo_.push_back(prev_);
  if (undo_.size() > 80)
    undo_.erase(undo_.begin());
}

void CardFace::take_restore_point()
{
  restore_.index = index();
  restore_.body = body();
  prev_ = restore_;
  undo_.clear();
}

bool CardFace::can_restore() const
{
  return index() != restore_.index || body() != restore_.body;
}

bool CardFace::can_undo() const
{
  return !undo_.empty();
}

void CardFace::restore()
{
  undoing_ = true;
  suppress_ = true;
  index_.set_text(restore_.index);
  body_buf_->set_text(restore_.body);
  suppress_ = false;
  undoing_ = false;
  prev_ = restore_;
  undo_.clear();
  signal_index_changed_.emit();
  signal_body_changed_.emit();
}

bool CardFace::undo()
{
  if (undo_.empty())
    return false;
  const Snap snap = undo_.back();
  undo_.pop_back();
  undoing_ = true;
  suppress_ = true;
  index_.set_text(snap.index);
  body_buf_->set_text(snap.body);
  suppress_ = false;
  undoing_ = false;
  prev_ = snap;
  signal_index_changed_.emit();
  signal_body_changed_.emit();
  return true;
}

void CardFace::apply_appearance(const std::string& family, int size_pt, int weight, int palette)
{
  const char* body_bg = "#F7F5EF";
  const char* body_fg = "#1A1A1A";
  const char* head_bg = "#E8E4D8";
  const char* head_fg = "#000000";
  const char* hit_bg = "#404040";
  const char* hit_fg = "#FFFFFF";
  const char* sel_bg = "#3D6AA8";
  const char* sel_fg = "#FFFFFF";
  if (palette == 0) {
    body_bg = "#FFFFFF";
    body_fg = "#000000";
    head_bg = "#E8E8E8";
    head_fg = "#000000";
  } else if (palette == 2) {
    body_bg = "#111111";
    body_fg = "#D8D8D8";
    head_bg = "#1A1A1A";
    head_fg = "#D8D8D8";
    hit_bg = "#C8C8C8";
    hit_fg = "#111111";
    sel_bg = "#8CB4E8";
    sel_fg = "#111111";
  }

  const int sz = std::max(8, std::min(size_pt, 32));
  const std::string fam = family.empty() ? std::string("Serif") : family;

  Pango::FontDescription body_desc;
  body_desc.set_family(fam);
  body_desc.set_size(sz * Pango::SCALE);
  body_desc.set_weight(static_cast<Pango::Weight>(weight));
  body_.override_font(body_desc);

  Pango::FontDescription idx_desc = body_desc;
  idx_desc.set_weight(Pango::WEIGHT_BOLD);
  index_.override_font(idx_desc);

  std::string fam_css = "\"";
  for (char c : fam) {
    if (c == '"' || c == '\\')
      fam_css += '\\';
    fam_css += c;
  }
  fam_css += "\"";

  char css[1536];
  std::snprintf(css, sizeof(css),
                ".yolodex-card {\n"
                "  background-color: %s;\n"
                "}\n"
                ".yolodex-card-index,\n"
                ".yolodex-card-index entry {\n"
                "  background-color: %s;\n"
                "  color: %s;\n"
                "  font-family: %s;\n"
                "  font-size: %dpt;\n"
                "  font-weight: bold;\n"
                "}\n"
                ".yolodex-card-body,\n"
                ".yolodex-card-body text {\n"
                "  background-color: %s;\n"
                "  color: %s;\n"
                "  font-family: %s;\n"
                "  font-size: %dpt;\n"
                "  font-weight: %d;\n"
                "}\n"
                ".yolodex-card-body text selection,\n"
                "textview.yolodex-card-body text selection {\n"
                "  background-color: %s;\n"
                "  color: %s;\n"
                "}\n",
                body_bg, head_bg, head_fg, fam_css.c_str(), sz, body_bg, body_fg, fam_css.c_str(),
                sz, weight, sel_bg, sel_fg);

  if (!chrome_css_) {
    chrome_css_ = Gtk::CssProvider::create();
    inner_.get_style_context()->add_provider(chrome_css_,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 50);
    index_.get_style_context()->add_provider(chrome_css_,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 50);
    body_.get_style_context()->add_provider(chrome_css_,
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 50);
    Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), chrome_css_,
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 50);
  }
  try {
    chrome_css_->load_from_data(css);
  } catch (const Glib::Error&) {
  }

  Gdk::RGBA bg_rgba, fg_rgba, hbg, hfg, sbg, sfg;
  bg_rgba.set(body_bg);
  fg_rgba.set(body_fg);
  hbg.set(head_bg);
  hfg.set(head_fg);
  sbg.set(sel_bg);
  sfg.set(sel_fg);
  inner_.override_background_color(bg_rgba);
  body_.override_background_color(bg_rgba);
  body_.override_color(fg_rgba);
  body_.override_background_color(sbg, Gtk::STATE_FLAG_SELECTED);
  body_.override_color(sfg, Gtk::STATE_FLAG_SELECTED);
  index_.override_background_color(hbg);
  index_.override_color(hfg);

  if (auto t = body_buf_->get_tag_table()->lookup("find-hit")) {
    t->property_background() = hit_bg;
    t->property_foreground() = hit_fg;
  }
}

}  // namespace yolodex
