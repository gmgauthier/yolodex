/* SPDX-License-Identifier: Unlicense */

#include "find.hpp"

namespace yolodex {

bool find_in_text(const Glib::ustring& hay, const Glib::ustring& needle, int from, int& start,
                  int& length)
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
  start = static_cast<int>(pos);
  length = static_cast<int>(n.size());
  return true;
}

bool find_in_cards(const std::vector<Card>& cards, const Glib::ustring& needle, int row, int field,
                   int offset, FindHit& hit)
{
  const int n = static_cast<int>(cards.size());
  if (n == 0 || needle.empty())
    return false;
  if (row < 0 || row >= n)
    row = 0;
  const int slots = n * 2;
  const int start_slot = row * 2 + (field == 0 ? 0 : 1);
  /* One extra slot: after wrapping, the starting field is scanned again from
   * its front, so a match before the resume offset is still found. */
  for (int i = 0; i <= slots; ++i) {
    const int s = (start_slot + i) % slots;
    const int r = s / 2;
    const int f = s % 2;
    const int from = (i == 0) ? offset : 0;
    const Card& c = cards[static_cast<size_t>(r)];
    const Glib::ustring& hay = f == 0 ? c.index : c.body;
    int start = 0;
    int len = 0;
    if (!find_in_text(hay, needle, from, start, len))
      continue;
    hit.row = r;
    hit.in_index = f == 0;
    hit.offset = start;
    hit.length = len;
    return true;
  }
  return false;
}

}  // namespace yolodex
