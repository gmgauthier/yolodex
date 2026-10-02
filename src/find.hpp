/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "stack.hpp"

#include <glibmm/ustring.h>

#include <vector>

namespace yolodex {

/* Case-insensitive search of hay for needle from character offset `from`.
 * On a hit, start and length are character offsets into hay. */
bool find_in_text(const Glib::ustring& hay, const Glib::ustring& needle, int from, int& start,
                  int& length);

struct FindHit {
  int row = -1;
  bool in_index = false;
  int offset = 0;
  int length = 0;
};

/* Search every card's index then body, starting at (row, field, offset) and
 * wrapping round the whole stack, back to the front of the starting field.
 * field 0 is the index, 1 the body. */
bool find_in_cards(const std::vector<Card>& cards, const Glib::ustring& needle, int row, int field,
                   int offset, FindHit& hit);

}  // namespace yolodex
