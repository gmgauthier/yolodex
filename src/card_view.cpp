/* SPDX-License-Identifier: Unlicense */

#include "card_view.hpp"

#include <algorithm>

namespace yolodex {
namespace {

constexpr int kTabH = 22;
constexpr int kYStep = 18;
constexpr int kXStep = 10;
constexpr int kPad = 8;
constexpr int kMaxTabs = 12;

}  // namespace

CardView::CardView()
{
  set_hexpand(true);
  set_vexpand(false);
  add_events(Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK |
             Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK);
  set_size_request(-1, kPad);
}

void CardView::bind(const Stack& stack)
{
  tabs_.clear();
  selected_id_ = stack.selected_id();
  int sel_row = -1;
  int i = 0;
  for (const Card& c : stack.cards()) {
    Tab t;
    t.id = c.id;
    t.index = c.index.empty() ? Glib::ustring("Untitled") : c.index;
    if (c.id == selected_id_)
      sel_row = i;
    tabs_.push_back(std::move(t));
    ++i;
  }
  if (sel_row < 0)
    sel_row = static_cast<int>(tabs_.size()) - 1;
  int start = sel_row - (kMaxTabs - 1);
  if (start < 0)
    start = 0;
  std::vector<Tab> vis;
  for (int r = start; r < sel_row; ++r)
    vis.push_back(tabs_[static_cast<size_t>(r)]);
  tabs_ = std::move(vis);

  const int shown = static_cast<int>(tabs_.size());
  int h = kPad;
  if (shown > 0)
    h = kPad * 2 + kTabH + (shown - 1) * kYStep;
  set_size_request(-1, h);
  hover_id_ = -1;
  queue_draw();
}

void CardView::layout_tabs()
{
  const int w = get_allocated_width();
  const int n = static_cast<int>(tabs_.size());
  for (int k = 0; k < n; ++k) {
    const int x = kPad + k * kXStep;
    const int y = kPad + k * kYStep;
    const int tw = std::max(40, w - x - kPad);
    tabs_[static_cast<size_t>(k)].rect = Gdk::Rectangle(x, y, tw, kTabH);
  }
}

int CardView::hit_id(double x, double y) const
{
  for (int i = static_cast<int>(tabs_.size()) - 1; i >= 0; --i) {
    const auto& r = tabs_[static_cast<size_t>(i)].rect;
    if (x >= r.get_x() && x < r.get_x() + r.get_width() && y >= r.get_y() &&
        y < r.get_y() + r.get_height())
      return tabs_[static_cast<size_t>(i)].id;
  }
  return -1;
}

void CardView::draw_tab(const Cairo::RefPtr<Cairo::Context>& cr, const Tab& tab, bool hover,
                        bool front)
{
  const double x = tab.rect.get_x();
  const double y = tab.rect.get_y();
  const double w = tab.rect.get_width();
  const double h = tab.rect.get_height();
  if (hover)
    cr->set_source_rgb(0.769, 0.769, 0.737);
  else if (front)
    cr->set_source_rgb(0.969, 0.961, 0.937);
  else
    cr->set_source_rgb(0.910, 0.894, 0.847);
  cr->rectangle(x, y, w, h);
  cr->fill();
  cr->set_source_rgb(0.25, 0.25, 0.25);
  cr->set_line_width(1.0);
  cr->rectangle(x + 0.5, y + 0.5, w - 1.0, h - 1.0);
  cr->stroke();

  auto layout = create_pango_layout(tab.index);
  Pango::FontDescription desc;
  desc.set_family("Sans");
  desc.set_weight(Pango::WEIGHT_BOLD);
  desc.set_size(11 * Pango::SCALE);
  layout->set_font_description(desc);
  layout->set_ellipsize(Pango::ELLIPSIZE_END);
  layout->set_width(static_cast<int>((w - 16) * Pango::SCALE));
  cr->set_source_rgb(0, 0, 0);
  cr->move_to(x + 8, y + 4);
  layout->show_in_cairo_context(cr);
}

bool CardView::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  const Gtk::Allocation alloc = get_allocation();
  cr->set_source_rgb(0.902, 0.902, 0.882);
  cr->rectangle(0, 0, alloc.get_width(), alloc.get_height());
  cr->fill();
  layout_tabs();
  const int n = static_cast<int>(tabs_.size());
  for (int k = 0; k < n; ++k) {
    const Tab& tab = tabs_[static_cast<size_t>(k)];
    draw_tab(cr, tab, tab.id == hover_id_, k == n - 1);
  }
  return true;
}

bool CardView::on_button_press_event(GdkEventButton* event)
{
  if (!event || event->button != 1)
    return false;
  layout_tabs();
  const int id = hit_id(event->x, event->y);
  if (id < 1)
    return false;
  signal_card_chosen_.emit(id);
  return true;
}

bool CardView::on_motion_notify_event(GdkEventMotion* event)
{
  if (!event)
    return false;
  layout_tabs();
  const int id = hit_id(event->x, event->y);
  if (id != hover_id_) {
    hover_id_ = id;
    queue_draw();
  }
  return false;
}

bool CardView::on_leave_notify_event(GdkEventCrossing* event)
{
  if (event && event->detail == GDK_NOTIFY_INFERIOR)
    return false;
  if (hover_id_ != -1) {
    hover_id_ = -1;
    queue_draw();
  }
  return false;
}

bool CardView::on_scroll_event(GdkEventScroll* event)
{
  if (!event)
    return false;
  int step = 0;
  if (event->direction == GDK_SCROLL_UP)
    step = -1;
  else if (event->direction == GDK_SCROLL_DOWN)
    step = 1;
  else if (event->direction == GDK_SCROLL_SMOOTH) {
    if (event->delta_y < -0.1)
      step = -1;
    else if (event->delta_y > 0.1)
      step = 1;
  }
  if (step == 0)
    return false;
  signal_step_.emit(step);
  return true;
}

}  // namespace yolodex
