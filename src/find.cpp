/* SPDX-License-Identifier: Unlicense */

#include "find.hpp"

#include <vector>

namespace yolodex {

bool find_in_text(const Glib::ustring& hay, const Glib::ustring& needle, int from, int& start,
                  int& length)
{
  const Glib::ustring n = needle.casefold();
  if (n.empty())
    return false;
  if (from < 0)
    from = 0;
  if (static_cast<Glib::ustring::size_type>(from) > hay.size())
    return false;
  /* Fold one character at a time and remember which original character each
   * folded character came from. "ß" -> "ss", "ﬁ" -> "fi", "İ" -> "i̇" grow,
   * so offsets in the folded string are not offsets in hay. */
  Glib::ustring folded;
  std::vector<int> origin;
  origin.reserve(hay.size() + 8);
  int index = 0;
  int folded_from = -1;
  for (auto it = hay.begin(); it != hay.end(); ++it, ++index) {
    if (index == from)
      folded_from = static_cast<int>(folded.size());
    const Glib::ustring f = Glib::ustring(1, *it).casefold();
    folded += f;
    for (Glib::ustring::size_type k = 0; k < f.size(); ++k)
      origin.push_back(index);
  }
  if (folded_from < 0)
    folded_from = static_cast<int>(folded.size());
  const auto pos = folded.find(n, static_cast<Glib::ustring::size_type>(folded_from));
  if (pos == Glib::ustring::npos)
    return false;
  const int first = origin[pos];
  const int last = origin[pos + n.size() - 1];
  start = first;
  length = last - first + 1;
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
